import tkinter as tk
from tkinter import filedialog, scrolledtext
import subprocess
import os

# -----------------------------
# Info storage — one .txt file per parameter
# stored in ./param_info/ folder next to this script
# -----------------------------
INFO_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "param_info")
os.makedirs(INFO_DIR, exist_ok=True)

def _info_path(key):
    safe = key.replace("/", "_").replace("\\", "_")
    return os.path.join(INFO_DIR, safe + ".txt")

def get_info(key):
    path = _info_path(key)
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            return f.read()
    return ""

def set_info(key, text):
    with open(_info_path(key), "w", encoding="utf-8") as f:
        f.write(text)


# -----------------------------
# Info button factory
# -----------------------------
def make_info_button(parent, key, row, col):
    def on_click():
        current = get_info(key)
        dialog = tk.Toplevel(parent)
        dialog.title(f"Info — {key}")
        dialog.geometry("420x210")
        dialog.resizable(True, True)
        dialog.grab_set()

        tk.Label(dialog, text=f"Parameter: {key}",
                 font=("TkDefaultFont", 10, "bold")).pack(anchor="w", padx=10, pady=(10, 2))
        tk.Label(dialog, text="Description / notes:").pack(anchor="w", padx=10)

        text_box = scrolledtext.ScrolledText(dialog, height=6, wrap="word")
        text_box.insert("1.0", current)
        text_box.pack(fill="both", expand=True, padx=10, pady=5)

        def save():
            set_info(key, text_box.get("1.0", "end-1c"))
            dialog.destroy()

        btn_frame = tk.Frame(dialog)
        btn_frame.pack(fill="x", padx=10, pady=(0, 10))
        tk.Button(btn_frame, text="Save", command=save).pack(side="right", padx=4)
        tk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side="right")

    btn = tk.Button(parent, text="ℹ", width=2, relief="flat",
                    fg="#2255aa", font=("TkDefaultFont", 9, "bold"),
                    cursor="question_arrow", command=on_click)
    btn.grid(row=row, column=col, padx=(2, 0))
    return btn


# -----------------------------
# Helpers
# -----------------------------
def browse_file(entry):
    path = filedialog.askopenfilename()
    if path:
        entry.delete(0, tk.END)
        entry.insert(0, path)

def save_file(entry):
    path = filedialog.asksaveasfilename()
    if path:
        entry.delete(0, tk.END)
        entry.insert(0, path)

def append_output(text):
    output_box.insert(tk.END, text + "\n")
    output_box.see(tk.END)


# -----------------------------
# Topo inputs (shared)
# -----------------------------
def get_topo_inputs():
    return [
        topo_time_entry.get(),
        topo_lat_entry.get(),
        topo_lon_entry.get(),
    ]


# -----------------------------
# RUNNERS
# -----------------------------
def run_process(inputs, label="pipeline"):
    append_output(f"\n🚀 Running {label}...\n")
    try:
        process = subprocess.Popen(
            ["python", "RAG_Pulsar_Software.py"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        output, error = process.communicate("\n".join(inputs) + "\n")
        append_output(output)
        if error:
            append_output(error)
        append_output("✅ Done\n")
    except Exception as e:
        append_output(f"❌ Error: {e}")


def run_pipeline():
    inputs = ["1"]

    inputs.append("y" if var_rtl_sdr.get() else "n")
    inputs.append("y" if var_rtlchannel.get() else "n")
    inputs.append("y" if var_pulsar_det.get() else "n")
    inputs.append("y" if var_pul_plot.get() else "n")

    if var_rtlchannel.get():
        inputs.append(file2_entry.get())
        inputs.append(file3_entry.get())
        for e in params2_entries:
            inputs.append(e.get())

    inputs.extend(get_topo_inputs())

    if var_pulsar_det.get():
        for e in params1_entries:
            inputs.append(e.get())

    run_process(inputs, "full pipeline")


def run_rtlchannel_only():
    inputs = ["1", "n", "y", "n", "n"]
    inputs.append(file2_entry.get())
    inputs.append(file3_entry.get())
    for e in params2_entries:
        inputs.append(e.get())
    inputs.extend(get_topo_inputs())
    run_process(inputs, "RTLChannel4bin")


def run_pulsar_det_only():
    inputs = ["1", "n", "n", "y", "n"]
    inputs.extend(get_topo_inputs())
    for e in params1_entries:
        inputs.append(e.get())
    run_process(inputs, "pulsar_det_an")


def run_rtl_sdr_only():
    inputs = ["1", "y", "n", "n", "n"]
    inputs.extend(get_topo_inputs())
    run_process(inputs, "rtl_sdr")


def run_pul_plot_only():
    inputs = ["1", "n", "n", "n", "y"]
    inputs.extend(get_topo_inputs())
    run_process(inputs, "pul_plot")


# -----------------------------
# UI
# -----------------------------
root = tk.Tk()
root.title("Pulsar GUI Runner")
root.geometry("1000x950")

canvas = tk.Canvas(root)
scrollbar = tk.Scrollbar(root, orient="vertical", command=canvas.yview)
canvas.configure(yscrollcommand=scrollbar.set)
scrollbar.pack(side="right", fill="y")
canvas.pack(side="left", fill="both", expand=True)

main = tk.Frame(canvas, padx=10, pady=10)
canvas_window = canvas.create_window((0, 0), window=main, anchor="nw")

def on_frame_configure(event):
    canvas.configure(scrollregion=canvas.bbox("all"))

def on_canvas_configure(event):
    canvas.itemconfig(canvas_window, width=event.width)

main.bind("<Configure>", on_frame_configure)
canvas.bind("<Configure>", on_canvas_configure)

def on_mousewheel(event):
    canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
canvas.bind_all("<MouseWheel>", on_mousewheel)


# =============================
# PROGRAM SELECTION (checkboxes)
# =============================
sel_frame = tk.LabelFrame(main, text="Program Selection", padx=10, pady=10)
sel_frame.pack(fill="x", pady=5)

var_rtl_sdr    = tk.BooleanVar(value=False)
var_rtlchannel = tk.BooleanVar(value=True)
var_pulsar_det = tk.BooleanVar(value=True)
var_pul_plot   = tk.BooleanVar(value=False)

programs = [
    ("rtl_sdr",        var_rtl_sdr),
    ("RTLChannel4bin", var_rtlchannel),
    ("pulsar_det_an",  var_pulsar_det),
    ("pul_plot",       var_pul_plot),
]

for col, (label, var) in enumerate(programs):
    tk.Checkbutton(sel_frame, text=label, variable=var).grid(row=0, column=col, padx=12, sticky="w")


# =============================
# RTLChannel4bin
# =============================
frame2 = tk.LabelFrame(main, text="RTLChannel4bin", padx=10, pady=10)
frame2.pack(fill="x", pady=5)

tk.Label(frame2, text="Input file").grid(row=0, column=0, sticky="w")
file2_entry = tk.Entry(frame2)
file2_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame2, text="Browse", command=lambda: browse_file(file2_entry)).grid(row=0, column=2)
make_info_button(frame2, "RTLChannel4bin — Input file", row=0, col=3)

tk.Label(frame2, text="Output file").grid(row=1, column=0, sticky="w")
file3_entry = tk.Entry(frame2)
file3_entry.grid(row=1, column=1, sticky="ew")
tk.Button(frame2, text="Save As", command=lambda: save_file(file3_entry)).grid(row=1, column=2)
make_info_button(frame2, "RTLChannel4bin — Output file", row=1, col=3)

params2_info = [
    ("Sample Rate [MSPS]",     "2.4"),
    ("Data set rate [kHz]",    "1"),
    ("Number of Channels [/]", "16"),
]
params2_entries = []
for i, (name, default) in enumerate(params2_info):
    tk.Label(frame2, text=name).grid(row=2+i, column=0, sticky="w")
    e = tk.Entry(frame2)
    e.insert(0, default)
    e.grid(row=2+i, column=1, sticky="ew")
    params2_entries.append(e)
    make_info_button(frame2, f"RTLChannel4bin — {name}", row=2+i, col=3)

frame2.columnconfigure(1, weight=1)

tk.Button(frame2, text="▶ Run RTLChannel4bin", command=run_rtlchannel_only,
          fg="white", bg="#2255aa").grid(
    row=2+len(params2_info), column=0, columnspan=4, pady=(8, 2), sticky="ew")


# =============================
# TOPOBARY (shared)
# =============================
topo_frame = tk.LabelFrame(main, text="TopoBary  —  shared by all programs", padx=10, pady=10)
topo_frame.pack(fill="x", pady=5)

tk.Label(topo_frame, text="Observation time").grid(row=0, column=0, sticky="w")
topo_time_entry = tk.Entry(topo_frame)
topo_time_entry.insert(0, "2018-06-27T02:52:00.0")
topo_time_entry.grid(row=0, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Observation time", row=0, col=2)

tk.Label(topo_frame, text="Latitude").grid(row=1, column=0, sticky="w")
topo_lat_entry = tk.Entry(topo_frame)
topo_lat_entry.insert(0, "54.21")
topo_lat_entry.grid(row=1, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Latitude", row=1, col=2)

tk.Label(topo_frame, text="Longitude").grid(row=2, column=0, sticky="w")
topo_lon_entry = tk.Entry(topo_frame)
topo_lon_entry.insert(0, "-1.33")
topo_lon_entry.grid(row=2, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Longitude", row=2, col=2)

topo_frame.columnconfigure(1, weight=1)


# =============================
# pulsar_det_an
# =============================
frame1 = tk.LabelFrame(main, text="pulsar_det_an", padx=10, pady=10)
frame1.pack(fill="x", pady=5)

tk.Label(frame1, text="Input file").grid(row=0, column=0, sticky="w")
file1_entry = tk.Entry(frame1)
file1_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame1, text="Browse", command=lambda: browse_file(file1_entry)).grid(row=0, column=2)
make_info_button(frame1, "pulsar_det_an — Input file", row=0, col=3)

params1_info = [
    ("Param 1",                 "16"),
    ("Param 2",                 "1"),
    ("ATNF pulsar period [ms]", "714.47415"),
    ("Bins",                    "128"),
    ("Window Size",             "1024"),
    ("Threshold",               "6.5"),
    ("ATNF DM [pc/cm^3]",       "-26.7"),
    ("ppm offset",              "-1.3"),
    ("Factor",                  "6"),
    ("Mode",                    "1"),
    ("RF bandwidth",            "2.4"),
    ("RF centre frequency",     "422"),
    ("ppm range factor",        "50"),
    ("Start bin",               "0"),
    ("End bin",                 "127"),
]
params1_entries = []
for i, (name, default) in enumerate(params1_info):
    tk.Label(frame1, text=name).grid(row=1+i, column=0, sticky="w")
    e = tk.Entry(frame1)
    e.insert(0, default)
    e.grid(row=1+i, column=1, sticky="ew")
    params1_entries.append(e)
    make_info_button(frame1, f"pulsar_det_an — {name}", row=1+i, col=3)

frame1.columnconfigure(1, weight=1)

tk.Button(frame1, text="▶ Run pulsar_det_an", command=run_pulsar_det_only,
          fg="white", bg="#2255aa").grid(
    row=1+len(params1_info), column=0, columnspan=4, pady=(8, 2), sticky="ew")


# =============================
# RUN FULL PIPELINE
# =============================
tk.Button(main, text="🚀 Run Full Pipeline", command=run_pipeline,
          font=("TkDefaultFont", 11, "bold"), pady=6,
          fg="white", bg="#115500").pack(fill="x", pady=10)


# =============================
# OUTPUT
# =============================
output_frame = tk.LabelFrame(main, text="Output", padx=5, pady=5)
output_frame.pack(fill="both", expand=True)

output_box = scrolledtext.ScrolledText(output_frame, height=12)
output_box.pack(fill="both", expand=True)

root.mainloop()