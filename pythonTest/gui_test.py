import tkinter as tk
from tkinter import ttk

ITEM_COUNT = 8


class ItemRow:
    def __init__(self, parent, index):
        self.index = index

        self.enabled = tk.BooleanVar(value=True)
        self.edit_text = tk.StringVar(value=f"Item {index + 1}")
        self.label_text = tk.StringVar(value=self.edit_text.get())

        self.frame = ttk.Frame(parent, padding=5)
        self.frame.grid(row=index, column=0, sticky="ew", pady=2)
        self.frame.columnconfigure(3, weight=1)

        self.button = ttk.Button(
            self.frame,
            text=f"Action {index + 1}",
            command=self.on_button
        )
        self.button.grid(row=0, column=0, padx=5)

        self.checkbox = ttk.Checkbutton(
            self.frame,
            text="Enabled",
            variable=self.enabled,
            command=self.on_checkbox
        )
        self.checkbox.grid(row=0, column=1, padx=5)

        self.label = ttk.Label(
            self.frame,
            textvariable=self.label_text,
            width=20
        )
        self.label.grid(row=0, column=2, padx=5)

        self.entry = ttk.Entry(
            self.frame,
            textvariable=self.edit_text
        )
        self.entry.grid(row=0, column=3, padx=5, sticky="ew")

        # Update the label whenever the Entry content changes.
        self.edit_text.trace_add("write", self.on_entry_changed)

        # Run the row action when Enter is pressed.
        self.entry.bind("<Return>", lambda event: self.on_button())

    def on_entry_changed(self, *_):
        self.label_text.set(self.edit_text.get())

    def on_button(self):
        if self.enabled.get():
            self.label_text.set(f"Clicked: {self.edit_text.get()}")
            print(f"Row {self.index}: action clicked")
        else:
            self.label_text.set("Row is disabled")

    def on_checkbox(self):
        if self.enabled.get():
            self.button.state(["!disabled"])
            self.entry.state(["!disabled"])
            self.label_text.set(self.edit_text.get())
        else:
            self.button.state(["disabled"])
            self.entry.state(["disabled"])
            self.label_text.set("Disabled")

    def clear(self):
        self.edit_text.set("")

    def set_enabled(self, enabled):
        self.enabled.set(enabled)
        self.on_checkbox()


class Application(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("Dynamic Tkinter Rows")
        self.geometry("650x400")
        self.minsize(500, 250)

        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        self.create_top_row()
        self.create_item_area()

    def create_top_row(self):
        top_frame = ttk.Frame(self, padding=10)
        top_frame.grid(row=0, column=0, sticky="ew")

        ttk.Button(
            top_frame,
            text="Enable all",
            command=self.enable_all
        ).grid(row=0, column=0, padx=5)

        ttk.Button(
            top_frame,
            text="Clear all",
            command=self.clear_all
        ).grid(row=0, column=1, padx=5)

    def create_item_area(self):
        container = ttk.Frame(self, padding=(10, 0, 10, 10))
        container.grid(row=1, column=0, sticky="nsew")
        container.columnconfigure(0, weight=1)

        self.rows = []

        for index in range(ITEM_COUNT):
            row = ItemRow(container, index)
            self.rows.append(row)

    def enable_all(self):
        for row in self.rows:
            row.set_enabled(True)

    def clear_all(self):
        for row in self.rows:
            row.clear()


if __name__ == "__main__":
    app = Application()
    app.mainloop()