import tkinter as tk
from tkinter import filedialog, scrolledtext
import subprocess
import os

# -----------------------------
# Info storage — one .txt file per parameter
# Stored in ./param_info/ folder next to this script
# -----------------------------
INFO_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "param_info")
os.makedirs(INFO_DIR, exist_ok=True)

def _info_path(key):
    # Replace characters that are illegal in filenames
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
# Info button — opens a small popup with an editable text box (no save/cancel)
# Changes are written to file when the popup is closed
# -----------------------------
def make_info_button(parent, key, row, col):
    def on_click():
        dialog = tk.Toplevel(parent)
        dialog.title(f"ℹ  {key}")
        dialog.geometry("460x180")
        dialog.resizable(True, True)

        tk.Label(dialog, text=key,
                 font=("TkDefaultFont", 9, "bold")).pack(anchor="w", padx=10, pady=(8, 2))

        text_box = scrolledtext.ScrolledText(dialog, height=7, wrap="word",
                                             font=("TkDefaultFont", 9))
        text_box.insert("1.0", get_info(key))
        text_box.config(state="disabled")  # read-only; edit the .txt file directly
        text_box.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)

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
# UI SETUP
# -----------------------------
root = tk.Tk()
root.title("Pulsar GUI Runner")
root.geometry("1000x1100")

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
def on_mousewheel(event):
    canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

main.bind("<Configure>", on_frame_configure)
canvas.bind("<Configure>", on_canvas_configure)
canvas.bind_all("<MouseWheel>", on_mousewheel)


# =============================
# TITLE
# =============================
title_frame = tk.Frame(main)
title_frame.pack(fill="x", pady=(0, 5))

tk.Label(title_frame, text="RAG Pulsar Detection Pipeline",
         font=("TkDefaultFont", 16, "bold")).pack(side="left")
tk.Label(title_frame, text="RAG_Pulsar_Software.py",
         font=("TkDefaultFont", 9), fg="#666666").pack(
    side="left", padx=(12, 0), anchor="s", pady=(0, 3))

tk.Frame(main, height=1, bg="#cccccc").pack(fill="x", pady=(0, 8))


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
# rtl_sdr  (display only)
# =============================
rtlsdr_frame = tk.LabelFrame(main, text="rtl_sdr", padx=10, pady=10)
rtlsdr_frame.pack(fill="x", pady=5)

rtlsdr_desc = (
    "Interfaces with the RTL-SDR USB dongle to capture raw IQ samples from the antenna.\n\n"
    "Input →  RF signal from antenna via USB dongle\n"
    "Output → Raw binary IQ sample file (.bin) at the configured sample rate\n\n"
    "This is the first stage in the pipeline. It controls the receiver tuning frequency, "
    "gain, and sample rate. The output file is consumed directly by RTLChannel4bin."
)
rtlsdr_text = tk.Text(rtlsdr_frame, height=6, wrap="word", relief="flat",
                      bg=root.cget("bg"), font=("TkDefaultFont", 9),
                      state="normal", cursor="arrow")
rtlsdr_text.insert("1.0", rtlsdr_desc)
rtlsdr_text.config(state="disabled")
rtlsdr_text.pack(fill="x")

tk.Button(rtlsdr_frame, text="▶ Run rtl_sdr", command=run_rtl_sdr_only,
          fg="white", bg="#2255aa").pack(fill="x", pady=(8, 2))


# =============================
# TOPOBARY (shared, full width)
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
# 3-COLUMN ROW: RTLChannel4bin | pulsar_det_an | pul_plot
# =============================
cols_frame = tk.Frame(main)
cols_frame.pack(fill="both", expand=True, pady=5)
cols_frame.columnconfigure(0, weight=1, uniform="col")
cols_frame.columnconfigure(1, weight=1, uniform="col")
cols_frame.columnconfigure(2, weight=1, uniform="col")


# --- Column 0: RTLChannel4bin ---
frame2 = tk.LabelFrame(cols_frame, text="RTLChannel4bin", padx=8, pady=8)
frame2.grid(row=0, column=0, sticky="nsew", padx=(0, 4), pady=0)
frame2.columnconfigure(1, weight=1)

tk.Label(frame2, text="Input file").grid(row=0, column=0, sticky="w")
file2_entry = tk.Entry(frame2)
file2_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame2, text="…", width=2,
          command=lambda: browse_file(file2_entry)).grid(row=0, column=2)
make_info_button(frame2, "RTLChannel4bin — Input file", row=0, col=3)

tk.Label(frame2, text="Output file").grid(row=1, column=0, sticky="w")
file3_entry = tk.Entry(frame2)
file3_entry.grid(row=1, column=1, sticky="ew")
tk.Button(frame2, text="…", width=2,
          command=lambda: save_file(file3_entry)).grid(row=1, column=2)
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

tk.Button(frame2, text="▶ Run RTLChannel4bin", command=run_rtlchannel_only,
          fg="white", bg="#2255aa").grid(
    row=2+len(params2_info), column=0, columnspan=4, pady=(8, 2), sticky="ew")


# --- Column 1: pulsar_det_an ---
frame1 = tk.LabelFrame(cols_frame, text="pulsar_det_an", padx=8, pady=8)
frame1.grid(row=0, column=1, sticky="nsew", padx=2, pady=0)
frame1.columnconfigure(1, weight=1)

tk.Label(frame1, text="Input file").grid(row=0, column=0, sticky="w")
file1_entry = tk.Entry(frame1)
file1_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame1, text="…", width=2,
          command=lambda: browse_file(file1_entry)).grid(row=0, column=2)
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

tk.Button(frame1, text="▶ Run pulsar_det_an", command=run_pulsar_det_only,
          fg="white", bg="#2255aa").grid(
    row=1+len(params1_info), column=0, columnspan=4, pady=(8, 2), sticky="ew")


# --- Column 2: pul_plot (display only) ---
pulplot_frame = tk.LabelFrame(cols_frame, text="pul_plot", padx=8, pady=8)
pulplot_frame.grid(row=0, column=2, sticky="nsew", padx=(4, 0), pady=0)

pulplot_desc = (
    "Visualises the folded pulse profile and diagnostic plots produced by pulsar_det_an.\n\n"
    "Input →  Folded profile data file output by pulsar_det_an\n"
    "Output → Matplotlib figures: pulse profile, S/N vs DM, time-phase plot\n\n"
    "This is the final stage in the pipeline. It produces publication-quality plots of the "
    "detected pulse, dispersion measure curve, and frequency-time waterfall for inspection "
    "and verification of the detection."
)
pulplot_text = tk.Text(pulplot_frame, height=8, wrap="word", relief="flat",
                       bg=cols_frame.cget("bg"), font=("TkDefaultFont", 9),
                       state="normal", cursor="arrow")
pulplot_text.insert("1.0", pulplot_desc)
pulplot_text.config(state="disabled")
pulplot_text.pack(fill="x")

tk.Button(pulplot_frame, text="▶ Run pul_plot", command=run_pul_plot_only,
          fg="white", bg="#2255aa").pack(fill="x", pady=(8, 2))


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
