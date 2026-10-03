#!/usr/bin/env python3
"""Serial GUI for the ESP-NOW DMX fixture controller terminal.

The controller must expose its terminal on UART0 at 115200 8N1.  The GUI
uses the firmware's existing line commands (discover, status N, config N, etc.).
"""
from __future__ import annotations

import queue
import re
import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk
from dataclasses import dataclass, field
from typing import Callable, Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

BAUD = 115200
MAX_FIXTURES = 20
POLL_INTERVAL_MS = 1000
DMX_CHANNELS = 512
LOW_VOLTAGE_WARNING_V = 3.5
LOW_VOLTAGE_CRITICAL_V = 3.0
VOLTAGE_WARNING_COLOR = "#a87900"
VOLTAGE_CRITICAL_COLOR = "#c62828"
DMX_DRAG_PIXELS_PER_STEP = 2.0
DMX_SET_THROTTLE_MS = 80


@dataclass
class Fixture:
    index: int
    mac: str
    status: str = "Not yet refreshed"
    firmware: str = "—"
    battery: str = "--"
    battery_v: Optional[float] = None
    rssi: str = "—"
    packet_loss: str = "—"
    dmx_address: str = "—"
    config: dict[str, str] = field(default_factory=dict)


def parse_discovery(text: str) -> list[Fixture]:
    """Parse listPeers() output: '<index>:\t<mac>'; tolerate surrounding logs."""
    found: list[Fixture] = []
    seen: set[int] = set()
    for line in text.splitlines():
        match = re.search(r"(?:^|\s)(\d+)\s*:\s*((?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})", line)
        if match:
            index = int(match.group(1))
            if index not in seen:
                found.append(Fixture(index=index, mac=match.group(2).lower()))
                seen.add(index)
    return found[:MAX_FIXTURES]


def parse_status(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    patterns = {
        "firmware": r"Firmware\s*:\s*(\d+)",
        "battery": r"Battery\s*:\s*(\d+)\s*mV",
        "rssi": r"rssi\s*:\s*(-?\d+)\s*dB",
        "packet_loss": r"Packet loss\s*:\s*(\d+)",
    }
    for key, pattern in patterns.items():
        match = re.search(pattern, text, re.IGNORECASE)
        if match:
            result[key] = match.group(1)
    return result


def parse_dmx_values(text: str) -> list[int]:
    """Extract the values printed by the firmware's `dmx` command."""
    values = [int(token) for token in re.findall(r"(?<![A-Za-z])\d+", text)]
    return [value for value in values[:DMX_CHANNELS] if 0 <= value <= 255]


def parse_config(text: str) -> dict[str, str]:
    """Parse the tab-separated row printed by showConfigFromPeerList()."""
    result: dict[str, str] = {}
    fields = {
        "wifi": r"wifi\s*ch\s*:\s*(-?\d+)",
        "address": r"adr\s*:\s*(-?\d+)",
        "count": r"ch\s*count\s*:\s*(-?\d+)",
        "personality": r"pers\s*:\s*(-?\d+)",
    }
    for key, pattern in fields.items():
        match = re.search(pattern, text, re.IGNORECASE)
        if match:
            result[key] = match.group(1)
    # Gamma values are emitted in order in one tab-separated field.
    for i in range(4):
        gamma_field = re.search(r"Gamma\s*:\s*([^\t\r\n]+)", text, re.IGNORECASE)
        if gamma_field:
            values = re.findall(r"-?\d+", gamma_field.group(1))
            if i < len(values):
                result[f"gamma{i}"] = values[i]
    return result


class SerialWorker(threading.Thread):
    """Owns pyserial I/O so the Tk event loop never blocks."""
    def __init__(self, output: queue.Queue):
        super().__init__(daemon=True)
        self.output = output
        self.tasks: queue.Queue = queue.Queue()
        self.stop_event = threading.Event()
        self.port = None
        self.start()

    def submit(self, callback: Callable, *args):
        self.tasks.put((callback, args))

    def run(self):
        while not self.stop_event.is_set():
            try:
                callback, args = self.tasks.get(timeout=0.1)
            except queue.Empty:
                continue
            try:
                result = callback(*args)
                self.output.put(("result", result))
            except Exception as exc:  # surfaced in the GUI
                self.output.put(("error", str(exc)))

    def connect_port(self, name: str):
        if serial is None:
            raise RuntimeError("pyserial is not installed. Run: python -m pip install pyserial")
        if self.port and self.port.is_open:
            self.port.close()
        self.port = serial.Serial(name, BAUD, timeout=0.05, write_timeout=1)
        self.port.reset_input_buffer()
        return ("connected", name)

    def disconnect(self):
        if self.port and self.port.is_open:
            self.port.close()
        self.port = None
        return ("disconnected", "")

    def command(self, command: str, max_wait: float = 2.5) -> str:
        if not self.port or not self.port.is_open:
            raise RuntimeError("Connect to the fixture controller first.")
        self.port.reset_input_buffer()
        self.port.write((command.strip() + "\n").encode("ascii"))
        self.port.flush()
        # Wait for output, then stop after a short quiet period. A hard limit
        # protects the GUI when a fixture does not reply.
        deadline = time.monotonic() + max_wait
        last_data = None
        chunks: list[bytes] = []
        while time.monotonic() < deadline:
            data = self.port.read(512)
            if data:
                chunks.append(data)
                last_data = time.monotonic()
            elif last_data is not None and time.monotonic() - last_data >= 0.2:
                break
        return b"".join(chunks).decode("utf-8", errors="replace").replace("\r", "")


class FixtureManager(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("DMX Fixture Manager")
        self.geometry("1120x570")
        self.minsize(900, 420)
        self.output: queue.Queue = queue.Queue()
        self.worker = SerialWorker(self.output)
        self.fixtures: list[Fixture] = []
        self.rows: dict[int, dict[str, tk.Widget]] = {}
        self.auto_refresh = tk.BooleanVar(value=False)
        self.next_fixture = 0
        self.poll_busy = False
        self.dmx_window: Optional[tk.Toplevel] = None
        self.dmx_cells: list[tk.Label] = []
        self.dmx_values: list[int] = [0] * DMX_CHANNELS
        self.dmx_last_update: Optional[ttk.Label] = None
        self.dmx_drag: Optional[dict[str, int | float]] = None
        self.dmx_pending_values: dict[int, int] = {}
        self.dmx_flush_scheduled = False
        self._build()
        self._refresh_ports()
        self.after(80, self._drain_results)
        self.after(POLL_INTERVAL_MS, self._poll_tick)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build(self):
        top = ttk.Frame(self, padding=10)
        top.pack(fill="x")
        ttk.Label(top, text="Serial port:").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=30, state="readonly")
        self.port_combo.pack(side="left", padx=(6, 4))
        ttk.Button(top, text="↻", width=3, command=self._refresh_ports).pack(side="left")
        self.connect_button = ttk.Button(top, text="Connect", command=self._toggle_connection)
        self.connect_button.pack(side="left", padx=(6, 14))
        ttk.Button(top, text="Discover", command=self._discover).pack(side="left")
        ttk.Button(top, text="DMX table", command=self._open_dmx_table).pack(side="left", padx=(6, 0))
        ttk.Button(top, text="Refresh status", command=self._refresh_all).pack(side="left", padx=6)
        ttk.Checkbutton(top, text="Auto refresh (one fixture per second)", variable=self.auto_refresh).pack(side="left", padx=10)
        self.connection_label = ttk.Label(top, text="Disconnected")
        self.connection_label.pack(side="right")

        ttk.Label(self, text="Fixtures", font=("TkDefaultFont", 14, "bold")).pack(anchor="w", padx=12, pady=(4, 6))
        table_frame = ttk.Frame(self, padding=(10, 0, 10, 4))
        table_frame.pack(fill="both", expand=True)
        self.table_canvas = tk.Canvas(table_frame, highlightthickness=0)
        yscroll = ttk.Scrollbar(table_frame, orient="vertical", command=self.table_canvas.yview)
        self.table_canvas.configure(yscrollcommand=yscroll.set)
        self.table_canvas.pack(side="left", fill="both", expand=True)
        yscroll.pack(side="right", fill="y")
        self.table_inner = ttk.Frame(self.table_canvas)
        self.table_window = self.table_canvas.create_window((0, 0), window=self.table_inner, anchor="nw")
        self.table_inner.bind("<Configure>", lambda _e: self.table_canvas.configure(scrollregion=self.table_canvas.bbox("all")))
        self.table_canvas.bind("<Configure>", lambda e: self.table_canvas.itemconfigure(self.table_window, width=e.width))
        self.columns = [
            ("Unit", 5), ("MAC address", 18), ("DMX address", 12), ("Status", 18),
            ("FW", 7), ("Voltage", 12), ("RSSI", 10), ("Packet loss", 12), ("Actions", 16),
        ]
        for c, (heading, weight) in enumerate(self.columns):
            ttk.Label(self.table_inner, text=heading, style="Treeview.Heading", anchor="center").grid(row=0, column=c, sticky="ew", padx=2, pady=(0, 3))
            self.table_inner.columnconfigure(c, weight=weight, minsize=48)
        ttk.Label(self, text="Select a fixture and use IDENT / CONFIG on its row. Serial: 115200 8N1.", foreground="#555").pack(anchor="w", padx=12, pady=(0, 8))
        self.log = tk.Text(self, height=5, wrap="word", state="disabled")
        self.log.pack(fill="x", padx=10, pady=(0, 10))

    def _log(self, text: str):
        self.log.configure(state="normal")
        self.log.insert("end", text.rstrip() + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _refresh_ports(self):
        if list_ports is None:
            ports = []
        else:
            ports = [p.device for p in list_ports.comports()]
        self.port_combo["values"] = ports
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def _toggle_connection(self):
        if self.connection_label.cget("text").startswith("Connected"):
            self.worker.submit(self.worker.disconnect)
        else:
            name = self.port_var.get().strip()
            if not name:
                messagebox.showerror("No serial port", "Select a serial port first.")
                return
            self.connect_button.configure(state="disabled")
            self.worker.submit(self.worker.connect_port, name)

    def _discover(self):
        if self.poll_busy:
            self._log("Please wait for the current serial request to finish.")
            return
        self.poll_busy = True
        self.worker.submit(self._discover_task)

    def _discover_task(self):
        # Discovery can be delayed while the controller waits for randomized
        # replies. Give that command a generous window before asking for its
        # canonical peer list.
        out = self.worker.command("discover", 15.0)
        out += self.worker.command("list", 3.0)
        fixtures = parse_discovery(out)
        configs = {}
        for fixture in fixtures:
            config_text = self.worker.command(f"config {fixture.index}", 3.0)
            configs[fixture.index] = parse_config(config_text)
        return ("discovery", (out, configs))

    def _refresh_all(self):
        if not self.fixtures:
            self._log("Discover fixtures first.")
            return
        self.poll_busy = True
        self.worker.submit(self._refresh_all_task, [(f.index, f.mac) for f in self.fixtures])

    def _refresh_all_task(self, fixtures):
        values = []
        for index, mac in fixtures:
            out = self.worker.command(f"status {index}", 1.8)
            values.append((index, mac, parse_status(out), out))
        return ("refresh_all", values)

    def _poll_tick(self):
        want_dmx = self._dmx_window_is_open()
        want_status = self.auto_refresh.get() and bool(self.fixtures)
        connected = self.connection_label.cget("text").startswith("Connected")
        dragging = self.dmx_drag is not None or bool(self.dmx_pending_values)
        if connected and not dragging and not self.poll_busy and (want_dmx or want_status):
            fixture = None
            if want_status:
                fixture = self.fixtures[self.next_fixture % len(self.fixtures)]
                self.next_fixture = (self.next_fixture + 1) % len(self.fixtures)
            self.poll_busy = True
            self.worker.submit(self._periodic_refresh_task, want_dmx, fixture)
        self.after(POLL_INTERVAL_MS, self._poll_tick)

    def _periodic_refresh_task(self, want_dmx: bool, fixture: Optional[Fixture]):
        dmx_values = None
        status = None
        if want_dmx:
            dmx_values = parse_dmx_values(self.worker.command("dmx", 2.0))
        if fixture is not None:
            out = self.worker.command(f"status {fixture.index}", 1.8)
            status = (fixture.index, fixture.mac, parse_status(out), out)
        return ("periodic", (dmx_values, status))

    def _dmx_window_is_open(self) -> bool:
        return self.dmx_window is not None and self.dmx_window.winfo_exists()

    def _open_dmx_table(self):
        if self._dmx_window_is_open():
            self.dmx_window.deiconify()
            self.dmx_window.lift()
            return
        win = tk.Toplevel(self)
        self.dmx_window = win
        win.title("DMX Universe")
        win.geometry("980x700")
        win.minsize(700, 450)
        win.protocol("WM_DELETE_WINDOW", self._close_dmx_table)

        bar = ttk.Frame(win, padding=(10, 8))
        bar.pack(fill="x")
        ttk.Label(bar, text="DMX channels 0–511 · values 0–255").pack(side="left")
        self.dmx_last_update = ttk.Label(bar, text="Waiting for controller…")
        self.dmx_last_update.pack(side="right")

        outer = ttk.Frame(win, padding=(10, 0, 10, 10))
        outer.pack(fill="both", expand=True)
        canvas = tk.Canvas(outer, highlightthickness=0)
        scroll = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=scroll.set)
        canvas.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        grid = ttk.Frame(canvas)
        canvas_window = canvas.create_window((0, 0), window=grid, anchor="nw")
        grid.bind("<Configure>", lambda _e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind("<Configure>", lambda e: canvas.itemconfigure(canvas_window, width=e.width))
        for col in range(16):
            grid.columnconfigure(col, weight=1, uniform="dmx")
        self.dmx_cells = []
        for channel in range(DMX_CHANNELS):
            row, col = divmod(channel, 16)
            cell = tk.Label(
                grid, text=f"CH {channel:03d}\n0", width=7, height=2,
                font=("Menlo", 10), relief="groove", borderwidth=1,
                bg="#f1f3f5", fg="#202124", justify="center",
            )
            cell.grid(row=row, column=col, sticky="nsew", padx=2, pady=2)
            cell.bind("<ButtonPress-1>", lambda event, ch=channel: self._begin_dmx_drag(event, ch))
            cell.bind("<B1-Motion>", self._move_dmx_drag)
            cell.bind("<ButtonRelease-1>", self._end_dmx_drag)
            self.dmx_cells.append(cell)

    def _begin_dmx_drag(self, event, channel: int):
        if not self.connection_label.cget("text").startswith("Connected"):
            self._log("Connect to the controller before editing DMX values.")
            return "break"
        self.dmx_drag = {
            "channel": channel,
            "start_y": event.y_root,
            "start_value": self.dmx_values[channel],
        }
        return "break"

    def _move_dmx_drag(self, event):
        if self.dmx_drag is None:
            return "break"
        channel = int(self.dmx_drag["channel"])
        start_y = float(self.dmx_drag["start_y"])
        start_value = int(self.dmx_drag["start_value"])
        delta = round((start_y - event.y_root) / DMX_DRAG_PIXELS_PER_STEP)
        value = max(0, min(255, start_value + delta))
        if value != self.dmx_values[channel]:
            self.dmx_values[channel] = value
            self.dmx_cells[channel].configure(text=f"CH {channel:03d}\n{value}")
            self.dmx_pending_values[channel] = value
            if not self.dmx_flush_scheduled:
                self.dmx_flush_scheduled = True
                self.after(DMX_SET_THROTTLE_MS, self._flush_dmx_set)
        return "break"

    def _end_dmx_drag(self, _event):
        self.dmx_drag = None
        if self.dmx_pending_values:
            self._flush_dmx_set()
        return "break"

    def _flush_dmx_set(self):
        self.dmx_flush_scheduled = False
        if not self.dmx_pending_values:
            return
        if self.poll_busy:
            self.dmx_flush_scheduled = True
            self.after(25, self._flush_dmx_set)
            return
        channel, value = next(iter(self.dmx_pending_values.items()))
        del self.dmx_pending_values[channel]
        self.poll_busy = True
        self.worker.submit(self._set_dmx_task, channel, value)

    def _set_dmx_task(self, channel: int, value: int):
        raw = self.worker.command(f"dmx {channel} {value}", 1.0)
        return ("dmx_set", (channel, value, raw))

    def _close_dmx_table(self):
        if self.dmx_window is not None:
            self.dmx_window.destroy()
        self.dmx_window = None
        self.dmx_cells = []
        self.dmx_last_update = None
        self.dmx_drag = None
        self.dmx_pending_values.clear()

    def _update_dmx_table(self, values: list[int]):
        if not self._dmx_window_is_open() or not values:
            return
        for channel, value in enumerate(values[:len(self.dmx_cells)]):
            if channel in self.dmx_pending_values or (self.dmx_drag is not None and int(self.dmx_drag["channel"]) == channel):
                continue
            self.dmx_values[channel] = value
            cell = self.dmx_cells[channel]
            cell.configure(text=f"CH {channel:03d}\n{value}")
        if self.dmx_last_update is not None:
            self.dmx_last_update.configure(text=f"Updated {time.strftime('%H:%M:%S')}")

    def _drain_results(self):
        while True:
            try:
                kind, result = self.output.get_nowait()
            except queue.Empty:
                break
            self.poll_busy = False
            if kind == "error":
                self.connect_button.configure(state="normal")
                self._log("Serial error: " + result)
                continue
            if not isinstance(result, tuple):
                continue
            action, payload = result
            if action == "connected":
                self.connection_label.configure(text=f"Connected: {payload}")
                self.connect_button.configure(text="Disconnect", state="normal")
                self._log(f"Connected to {payload} at {BAUD} baud.")
            elif action == "disconnected":
                self.connection_label.configure(text="Disconnected")
                self.connect_button.configure(text="Connect", state="normal")
                self._log("Disconnected.")
            elif action == "discovery":
                raw, configs = payload
                self.fixtures = parse_discovery(raw)
                for fixture in self.fixtures:
                    fixture.config.update(configs.get(fixture.index, {}))
                    fixture.dmx_address = fixture.config.get("address", "—")
                self._render_fixtures()
                self.next_fixture = 0
                self._log(f"Discovered {len(self.fixtures)} fixture(s)." if self.fixtures else "No fixtures found.")
                if not self.fixtures:
                    excerpt = raw.strip() or "(the controller returned no serial text)"
                    self._log("Controller reply: " + excerpt[:1800].replace("\n", " | "))
                    self._log("Check that the selected port is the controller UART at 115200 8N1.")
                if len(parse_discovery(raw)) >= MAX_FIXTURES:
                    self._log(f"Showing the first {MAX_FIXTURES} fixtures.")
            elif action == "status":
                self._apply_status(payload)
            elif action == "periodic":
                dmx_values, status = payload
                if dmx_values is not None:
                    self._update_dmx_table(dmx_values)
                    if not dmx_values:
                        self._log("DMX refresh returned no channel values.")
                if status is not None:
                    self._apply_status(status)
            elif action == "dmx_set":
                if self.dmx_pending_values:
                    self._flush_dmx_set()
            elif action == "refresh_all":
                for item in payload:
                    self._apply_status(item)
                self._log("Manual status refresh complete.")
            elif action == "config":
                index, config, raw, open_view = payload
                fixture = self._by_index(index)
                if fixture:
                    fixture.config.update(config)
                    if "address" in config:
                        fixture.dmx_address = config["address"]
                    self._render_fixtures()
                if open_view:
                    self._show_config(index, config, raw)
            elif action == "log":
                message, _raw = payload
                self._log(message)
            elif action == "set":
                index, key, value, raw = payload
                self._log(f"Set {key} to {value} on fixture {index}.")
                self._request_config(index, open_view=False)
        self.after(80, self._drain_results)

    def _by_index(self, index: int) -> Optional[Fixture]:
        return next((f for f in self.fixtures if f.index == index), None)

    def _render_fixtures(self):
        for child in self.table_inner.winfo_children():
            if int(child.grid_info().get("row", 0)) > 0:
                child.destroy()
        for r, f in enumerate(self.fixtures[:MAX_FIXTURES], start=1):
            values = (f.index, f.mac, f.dmx_address, f.status, f.firmware, f.battery, f.rssi, f.packet_loss)
            for c, value in enumerate(values):
                options = {}
                if c == 5 and f.battery_v is not None:
                    if f.battery_v < LOW_VOLTAGE_CRITICAL_V:
                        options["foreground"] = VOLTAGE_CRITICAL_COLOR
                    elif f.battery_v < LOW_VOLTAGE_WARNING_V:
                        options["foreground"] = VOLTAGE_WARNING_COLOR
                ttk.Label(self.table_inner, text=str(value), anchor="center", padding=(3, 7), **options).grid(row=r, column=c, sticky="ew", padx=2, pady=1)
            actions = ttk.Frame(self.table_inner)
            actions.grid(row=r, column=8, sticky="ew", padx=2, pady=1)
            ttk.Button(actions, text="IDENT", command=lambda i=f.index: self._identify(i)).pack(side="left", expand=True, fill="x", padx=(0, 3))
            ttk.Button(actions, text="CONFIG", command=lambda i=f.index: self._request_config(i, open_view=True)).pack(side="left", expand=True, fill="x")

    def _apply_status(self, payload):
        index, mac, values, raw = payload
        f = self._by_index(index)
        if not f or f.mac != mac:
            return
        if values:
            f.status = "Online"
            f.firmware = values.get("firmware", "--")
            batt = values.get("battery")
            if batt is not None:
                f.battery_v = int(batt) / 1000.0
                f.battery = f"{f.battery_v:.1f} V"
            else:
                f.battery_v = None
                f.battery = "--"
            rssi = values.get("rssi")
            f.rssi = f"{rssi} dB" if rssi is not None else "--"
            f.packet_loss = values.get("packet_loss", "--")
            self._log(f"Fixture {index}: status updated.")
        else:
            f.status = "Offline"
            f.firmware = "--"
            f.battery = "--"
            f.battery_v = None
            f.rssi = "--"
            f.packet_loss = "--"
            self._log(f"Fixture {index}: no status reply; marked offline.")
        self._render_fixtures()

    def _identify(self, index: int):
        if self.poll_busy:
            return
        self.poll_busy = True
        self.worker.submit(self._simple_command_task, f"ident {index}", f"Fixture {index} identified.")

    def _simple_command_task(self, command, message):
        raw = self.worker.command(command, 1.0)
        return ("log", (message, raw))

    def _request_config(self, index: int, open_view=True):
        if self.poll_busy:
            self._log("Wait for the current serial request to finish before opening config.")
            return
        self.poll_busy = True
        self.worker.submit(self._config_task, index, open_view)

    def _config_task(self, index, open_view):
        raw = self.worker.command(f"config {index}", 2.5)
        return ("config", (index, parse_config(raw), raw, open_view))

    def _show_config(self, index: int, config: dict[str, str], raw: str):
        win = tk.Toplevel(self)
        win.title(f"Fixture {index} configuration")
        win.transient(self)
        win.grab_set()
        body = ttk.Frame(win, padding=14)
        body.pack(fill="both", expand=True)
        fixture = self._by_index(index)
        ttk.Label(body, text=f"Fixture {index} · {fixture.mac if fixture else ''}", font=("TkDefaultFont", 12, "bold")).grid(row=0, column=0, columnspan=4, sticky="w", pady=(0, 10))
        fields = [("Wi-Fi channel", "wifi", "wifi"), ("DMX address", "address", "adr"), ("Channel count", "count", "count"), ("Personality", "personality", "pers"), ("Gamma 0", "gamma0", "gamma0"), ("Gamma 1", "gamma1", "gamma1"), ("Gamma 2", "gamma2", "gamma2"), ("Gamma 3", "gamma3", "gamma3")]
        entries = {}
        for r, (label, key, _command) in enumerate(fields, start=1):
            ttk.Label(body, text=label).grid(row=r, column=0, sticky="w", pady=3)
            var = tk.StringVar(value=config.get(key, ""))
            ent = ttk.Entry(body, textvariable=var, width=14)
            ent.grid(row=r, column=1, padx=8, pady=3)
            entries[key] = var
            ttk.Label(body, text=f"Current: {config.get(key, '—')}", foreground="#555").grid(row=r, column=2, sticky="w", padx=4)
            ttk.Button(body, text="Set", command=lambda k=key, v=var: self._set_config(index, k, v.get(), win)).grid(row=r, column=3, padx=6)
        ttk.Label(body, text="Values are sent using the firmware's existing config commands.", foreground="#555").grid(row=9, column=0, columnspan=4, sticky="w", pady=(10, 0))
        if not config:
            ttk.Label(body, text="No values parsed. Raw response: " + (raw.strip() or "(no reply)"), wraplength=460, foreground="#a33").grid(row=10, column=0, columnspan=4, sticky="w", pady=8)

    def _set_config(self, index: int, key: str, value: str, window):
        value = value.strip()
        if not value.isdigit() or not 0 <= int(value) <= 65535:
            messagebox.showerror("Invalid value", "Enter a whole number from 0 to 65535.", parent=window)
            return
        commands = {"wifi": "wifi", "address": "adr", "count": "count", "personality": "pers", "gamma0": "gamma0", "gamma1": "gamma1", "gamma2": "gamma2", "gamma3": "gamma3"}
        self._submit_set(index, key, value, f"{commands[key]} {index} {value}")

    def _submit_set(self, index, key, value, command):
        if self.poll_busy:
            self._log("Wait for the current serial request to finish.")
            return
        self.poll_busy = True
        self.worker.submit(self._set_task, command, index, key, value)

    def _set_task(self, command, index, key, value):
        raw = self.worker.command(command, 1.0)
        return ("set", (index, key, value, raw))

    def _on_close(self):
        self.auto_refresh.set(False)
        self.worker.submit(self.worker.disconnect)
        self.worker.stop_event.set()
        self.destroy()


if __name__ == "__main__":
    app = FixtureManager()
    app.mainloop()
