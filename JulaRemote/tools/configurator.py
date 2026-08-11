import tkinter as tk
from tkinter import ttk
from time import time
from time import sleep
from threading import Thread

COLUMN_WIDTHS = (
    170,  # MAC
    70,   # Identify
    90,   # DMX address
    80,   # Channels
    90,   # Personality
    140,  # Values
    90,   # Battery
    70,   # RSSI
    90,   # Packet loss
)

COLUMN_HEADINGS = (
    "MAC",
    "Identify",
    "DMX address",
    "Channels",
    "Personality",
    "Values",
    "Battery",
    "RSSI",
    "Packet loss",
)


DEMO_DATA = [
    {"mac":[82,12,14,234,232,90],  "dmx_address":1,   "channel_count":4, "personality":0, "values":[255,128,64,0],   "battery_mv":3245, "rssi":-18, "packet_loss":0},
    {"mac":[83,42,141,224,232,90], "dmx_address":5,   "channel_count":4, "personality":1, "values":[0,255,255,32],   "battery_mv":3192, "rssi":-24, "packet_loss":1},
    {"mac":[84,42,141,224,232,90], "dmx_address":9,   "channel_count":1, "personality":0, "values":[180,0,0,0],      "battery_mv":3178, "rssi":-29, "packet_loss":0},
    {"mac":[85,42,141,224,232,90], "dmx_address":10,  "channel_count":1, "personality":0, "values":[64,0,0,0],       "battery_mv":3154, "rssi":-35, "packet_loss":2},
    {"mac":[86,12,14,234,232,90],  "dmx_address":20,  "channel_count":4, "personality":2, "values":[255,255,255,255],"battery_mv":3138, "rssi":-41, "packet_loss":0},
    {"mac":[87,42,141,224,232,90], "dmx_address":24,  "channel_count":4, "personality":1, "values":[32,64,96,128],   "battery_mv":3125, "rssi":-47, "packet_loss":4},
    {"mac":[88,42,141,224,232,90], "dmx_address":28,  "channel_count":1, "personality":0, "values":[255,0,0,0],      "battery_mv":3108, "rssi":-51, "packet_loss":1},
    {"mac":[89,42,141,224,232,90], "dmx_address":29,  "channel_count":1, "personality":1, "values":[127,0,0,0],      "battery_mv":3091, "rssi":-54, "packet_loss":0},
    {"mac":[90,12,14,234,232,90],  "dmx_address":40,  "channel_count":4, "personality":0, "values":[0,64,128,255],   "battery_mv":3079, "rssi":-58, "packet_loss":3},
    {"mac":[91,42,141,224,232,90], "dmx_address":44,  "channel_count":4, "personality":2, "values":[200,180,120,60], "battery_mv":3066, "rssi":-60, "packet_loss":7},
    {"mac":[92,42,141,224,232,90], "dmx_address":48,  "channel_count":1, "personality":0, "values":[10,0,0,0],       "battery_mv":3048, "rssi":-62, "packet_loss":1},
    {"mac":[93,42,141,224,232,90], "dmx_address":49,  "channel_count":1, "personality":0, "values":[220,0,0,0],      "battery_mv":3036, "rssi":-64, "packet_loss":0},
    {"mac":[94,12,14,234,232,90],  "dmx_address":60,  "channel_count":4, "personality":1, "values":[50,100,150,200], "battery_mv":3021, "rssi":-67, "packet_loss":2},
    {"mac":[95,42,141,224,232,90], "dmx_address":64,  "channel_count":4, "personality":0, "values":[90,40,170,230],  "battery_mv":3009, "rssi":-69, "packet_loss":5},
    {"mac":[96,42,141,224,232,90], "dmx_address":68,  "channel_count":1, "personality":2, "values":[255,0,0,0],      "battery_mv":2998, "rssi":-71, "packet_loss":9},
    {"mac":[97,42,141,224,232,90], "dmx_address":69,  "channel_count":1, "personality":1, "values":[1,0,0,0],        "battery_mv":2976, "rssi":-74, "packet_loss":0},
    {"mac":[98,12,14,234,232,90],  "dmx_address":80,  "channel_count":4, "personality":0, "values":[45,90,135,180],  "battery_mv":2954, "rssi":-77, "packet_loss":6},
    {"mac":[99,42,141,224,232,90], "dmx_address":84,  "channel_count":4, "personality":2, "values":[255,64,0,255],   "battery_mv":2929, "rssi":-80, "packet_loss":11},
    {"mac":[100,42,141,224,232,90],"dmx_address":88,  "channel_count":1, "personality":0, "values":[175,0,0,0],      "battery_mv":2895, "rssi":-83, "packet_loss":18},
    {"mac":[101,42,141,224,232,90],"dmx_address":89,  "channel_count":1, "personality":1, "values":[0,0,0,0],        "battery_mv":2868, "rssi":-87, "packet_loss":26},
]


def format_mac(mac):
    return ":".join(f"{value:02X}" for value in mac)


class ItemRow:
    def __init__(self, parent, index, item_data):
        self.parent = parent
        self.index = index
        self.grid_row = index + 1
        self.item_data = item_data

        self.dmx_address = tk.StringVar(
            value=str(item_data["dmx_address"])
        )
        self.channel_count = tk.StringVar(
            value=str(item_data["channel_count"])
        )
        self.personality = tk.StringVar(
            value=str(item_data["personality"])
        )

        self.values_text = tk.StringVar(
            value=", ".join(str(value) for value in item_data["values"])
        )
        self.battery_text = tk.StringVar(
            value=f'{item_data["battery_mv"]} mV'
        )
        self.rssi_text = tk.StringVar(
            value=f'{item_data["rssi"]} dBm'
        )
        self.packet_loss_text = tk.StringVar(
            value=str(item_data["packet_loss"])
        )

        self.widgets = []

        self.mac_label = ttk.Label(
            parent,
            text=format_mac(item_data["mac"]),
            anchor="w",
        )
        self.mac_label.grid(
            row=self.grid_row,
            column=0,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.mac_label)

        self.ident_button = ttk.Button(
            parent,
            text="Ident",
            command=self.identify,
        )
        self.ident_button.grid(
            row=self.grid_row,
            column=1,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.ident_button)

        self.dmx_address_entry = ttk.Entry(
            parent,
            textvariable=self.dmx_address,
            width=1,
        )
        self.dmx_address_entry.grid(
            row=self.grid_row,
            column=2,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.dmx_address_entry)

        self.channel_count_entry = ttk.Entry(
            parent,
            textvariable=self.channel_count,
            width=1,
        )
        self.channel_count_entry.grid(
            row=self.grid_row,
            column=3,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.channel_count_entry)

        self.personality_entry = ttk.Entry(
            parent,
            textvariable=self.personality,
            width=1,
        )
        self.personality_entry.grid(
            row=self.grid_row,
            column=4,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.personality_entry)

        self.values_label = ttk.Label(
            parent,
            textvariable=self.values_text,
            anchor="w",
        )
        self.values_label.grid(
            row=self.grid_row,
            column=5,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.values_label)

        self.battery_label = ttk.Label(
            parent,
            textvariable=self.battery_text,
            anchor="e",
        )
        self.battery_label.grid(
            row=self.grid_row,
            column=6,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.battery_label)

        self.rssi_label = ttk.Label(
            parent,
            textvariable=self.rssi_text,
            anchor="e",
        )
        self.rssi_label.grid(
            row=self.grid_row,
            column=7,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.rssi_label)

        self.packet_loss_label = ttk.Label(
            parent,
            textvariable=self.packet_loss_text,
            anchor="e",
        )
        self.packet_loss_label.grid(
            row=self.grid_row,
            column=8,
            sticky="ew",
            padx=3,
            pady=2,
        )
        self.widgets.append(self.packet_loss_label)

        self.dmx_address_entry.bind(
            "<Return>",
            self.apply_changes,
        )
        self.channel_count_entry.bind(
            "<Return>",
            self.apply_changes,
        )
        self.personality_entry.bind(
            "<Return>",
            self.apply_changes,
        )

    def identify(self):
        print(f"Identify: {format_mac(self.item_data['mac'])}")

        self.values_text.set("IDENTIFY")

        self.parent.after(
            1000,
            lambda: self.values_text.set(
                ", ".join(
                    str(value)
                    for value in self.item_data["values"]
                )
            ),
        )

    def apply_changes(self, event=None):
        try:
            self.item_data["dmx_address"] = int(
                self.dmx_address.get()
            )
            self.item_data["channel_count"] = int(
                self.channel_count.get()
            )
            self.item_data["personality"] = int(
                self.personality.get()
            )

            print(
                f"Updated {format_mac(self.item_data['mac'])}: "
                f"DMX={self.item_data['dmx_address']}, "
                f"channels={self.item_data['channel_count']}, "
                f"personality={self.item_data['personality']}"
            )

        except ValueError:
            print("DMX address, channels and personality must be integers.")

    def destroy(self):
        for widget in self.widgets:
            widget.destroy()


class Application(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("Device Configuration")
        self.geometry("950x500")
        self.minsize(800, 300)

        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        self.rows = []
        self.row_index = 0

        self.create_top_row()
        self.create_item_area()

    def create_top_row(self):
        top_frame = ttk.Frame(self, padding=10)
        top_frame.grid(
            row=0,
            column=0,
            sticky="ew",
        )

        ttk.Button(
            top_frame,
            text="Add item",
            command=self.add_item,
        ).grid(
            row=0,
            column=0,
            padx=(0, 5),
        )

        ttk.Button(
            top_frame,
            text="Clear items",
            command=self.clear_items,
        ).grid(
            row=0,
            column=1,
            padx=5,
        )

    def create_item_area(self):
        self.item_container = ttk.Frame(
            self,
            padding=(10, 0, 10, 10),
        )
        self.item_container.grid(
            row=1,
            column=0,
            sticky="nsew",
        )

        for column, width in enumerate(COLUMN_WIDTHS):
            self.item_container.columnconfigure(
                column,
                minsize=width,
                weight=0,
            )

        for column, heading in enumerate(COLUMN_HEADINGS):
            label = ttk.Label(
                self.item_container,
                text=heading,
                anchor="w",
                font=("TkDefaultFont", 10, "bold"),
            )
            label.grid(
                row=0,
                column=column,
                sticky="ew",
                padx=3,
                pady=(3, 6),
            )

    def add_item(self):
        chosen = int(time()) % len(DEMO_DATA)
        data = DEMO_DATA[chosen]

        for row in self.rows:
            if row.item_data["mac"] == data["mac"]:
                print("Device already exists.")
                return

        row = ItemRow(
            self.item_container,
            self.row_index,
            data.copy(),
        )

        self.rows.append(row)
        self.row_index += 1

    def clear_items(self):
        for row in self.rows:
            row.destroy()

        self.rows.clear()
        self.row_index = 0

    def live_data(self):
        while True:
            print("tick")
            for row in self.rows:
                row.item_data["rssi"] +=1
                row.rssi_text.set(f'{row.item_data["rssi"]} dBm')
                
            sleep(1)

if __name__ == "__main__":
    app = Application()
    thread = Thread(target=app.live_data, daemon=True)
    thread.start()
    app.mainloop()