import tkinter as tk
from tkinter import filedialog, scrolledtext
import subprocess
import os

# The script runs from the project root. lib/ is always next to it.
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
LIB_DIR  = os.path.join(ROOT_DIR, "lib")

# -----------------------------
# Info storage — one .txt file per parameter
# Stored in lib/param_info/ folder
# -----------------------------
INFO_DIR = os.path.join(LIB_DIR, "param_info")
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
        dialog.geometry("460x220")
        dialog.resizable(True, True)

        tk.Label(dialog, text=key,
                 font=("TkDefaultFont", 9, "bold")).pack(anchor="w", padx=10, pady=(8, 2))

        text_box = scrolledtext.ScrolledText(dialog, height=7, wrap="word",
                                             font=("TkDefaultFont", 9))
        text_box.insert("1.0", get_info(key))
        text_box.config(state="disabled")
        text_box.pack(fill="both", expand=True, padx=10, pady=(0, 4))

        btn_frame = tk.Frame(dialog)
        btn_frame.pack(fill="x", padx=10, pady=(0, 8))

        editing = [False]

        def toggle_edit():
            if not editing[0]:
                text_box.config(state="normal", bg="white")
                edit_btn.config(text="💾 Save", fg="white", bg="#115500")
                editing[0] = True
            else:
                set_info(key, text_box.get("1.0", "end-1c"))
                text_box.config(state="disabled", bg=dialog.cget("bg"))
                edit_btn.config(text="✏ Edit", fg="#2255aa", bg=btn_frame.cget("bg"))
                editing[0] = False

        edit_btn = tk.Button(btn_frame, text="✏ Edit", fg="#2255aa",
                             font=("TkDefaultFont", 9), command=toggle_edit)
        edit_btn.pack(side="left")

        def on_close():
            if editing[0]:
                set_info(key, text_box.get("1.0", "end-1c"))
            dialog.destroy()

        tk.Button(btn_frame, text="Close", command=on_close).pack(side="right")
        dialog.protocol("WM_DELETE_WINDOW", on_close)

    btn = tk.Button(parent, text="ℹ", width=2, relief="flat",
                    fg="#2255aa", font=("TkDefaultFont", 9, "bold"),
                    cursor="question_arrow", command=on_click)
    btn.grid(row=row, column=col, padx=(2, 0))
    return btn


# -----------------------------
# Section info buttons (for block titles)
# Stored in ./section_info/ folder next to this script
# -----------------------------
SECTION_INFO_DIR = os.path.join(LIB_DIR, "section_info")
os.makedirs(SECTION_INFO_DIR, exist_ok=True)

README_PATH = os.path.join(LIB_DIR, "README.txt")

def _section_info_path(key):
    safe = key.replace("/", "_").replace("\\", "_").replace(" ", "_")
    return os.path.join(SECTION_INFO_DIR, safe + ".txt")

def get_section_info(key):
    path = _section_info_path(key)
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            return f.read()
    return f"No description yet.\n\nEdit:  section_info/{os.path.basename(_section_info_path(key))}"

def make_section_info_button(parent, key, **pack_kwargs):
    """A small ℹ button for block/section titles. Uses pack geometry."""
    def on_click():
        dialog = tk.Toplevel(parent)
        dialog.title(f"ℹ  {key}")
        dialog.geometry("500x220")
        dialog.resizable(True, True)
        dialog.attributes("-topmost", True)

        tk.Label(dialog, text=key,
                 font=("TkDefaultFont", 10, "bold")).pack(anchor="w", padx=10, pady=(8, 2))

        text_box = scrolledtext.ScrolledText(dialog, height=8, wrap="word",
                                             font=("TkDefaultFont", 9))
        text_box.insert("1.0", get_section_info(key))
        text_box.config(state="disabled")
        text_box.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)

    btn = tk.Button(parent, text="More info", width=6, relief="groove",
                    fg="#884400", font=("TkDefaultFont", 8, "bold"),
                    cursor="question_arrow", command=on_click)
    btn.pack(**pack_kwargs)
    return btn


def make_section_info_button_grid(parent, key, row, col, **grid_kwargs):
    """A small ℹ button for block/section titles. Uses grid geometry."""
    def on_click():
        dialog = tk.Toplevel(parent)
        dialog.title(f"ℹ  {key}")
        dialog.geometry("500x220")
        dialog.resizable(True, True)
        dialog.attributes("-topmost", True)

        tk.Label(dialog, text=key,
                 font=("TkDefaultFont", 10, "bold")).pack(anchor="w", padx=10, pady=(8, 2))

        text_box = scrolledtext.ScrolledText(dialog, height=8, wrap="word",
                                             font=("TkDefaultFont", 9))
        text_box.insert("1.0", get_section_info(key))
        text_box.config(state="disabled")
        text_box.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)

    btn = tk.Button(parent, text="More info", width=6, relief="groove",
                    fg="#884400", font=("TkDefaultFont", 8, "bold"),
                    cursor="question_arrow", command=on_click)
    btn.grid(row=row, column=col, **grid_kwargs)
    return btn


def show_readme():
    """Open a popup showing README.txt, or a helpful message if it doesn't exist."""
    dialog = tk.Toplevel()
    dialog.title("README — RAG Pulsar Detection Pipeline")
    dialog.geometry("700x500")
    dialog.resizable(True, True)
    dialog.attributes("-topmost", True)

    if os.path.exists(README_PATH):
        with open(README_PATH, "r", encoding="utf-8") as f:
            content = f.read()
    else:
        content = (
            "No README.txt found.\n\n"
            f"Create a file at:\n  {README_PATH}\n\n"
            "and it will appear here."
        )

    tk.Label(dialog, text="README", font=("TkDefaultFont", 11, "bold")).pack(
        anchor="w", padx=12, pady=(10, 2))
    tk.Label(dialog, text=README_PATH, font=("TkDefaultFont", 8),
             fg="#666666").pack(anchor="w", padx=12, pady=(0, 6))

    text_box = scrolledtext.ScrolledText(dialog, wrap="word", font=("TkDefaultFont", 9))
    text_box.insert("1.0", content)
    text_box.config(state="disabled")
    text_box.pack(fill="both", expand=True, padx=10, pady=(0, 10))

    dialog.protocol("WM_DELETE_WINDOW", dialog.destroy)


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
# Prompt order is defined by io.py. Key rules:
#   1. Chain selection: always "1"
#   2. Program selection: rtl_sdr [y/N], RTLChannel4bin [Y/n], pulsar_det_an [Y/n], pul_plot [Y/n]
#      The loop breaks when a program is deselected AFTER the chain has started,
#      so we only send y/n up to and including the last program we want.
#   3. RTLChannel4bin inputs: in_file, out_file, clock_rate, OP_data_set_rate, fft_points
#   4. TopoBary inputs are ONLY prompted when pulsar_det_an is selected (inside that case block)
#   5. pulsar_det_an inputs follow TopoBary, in_file is auto-set if RTLChannel4bin ran

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
        return output  # returned so callers (e.g. run_pipeline) can parse it
    except Exception as e:
        append_output(f"❌ Error: {e}")
        return ""

def run_pipeline():
    # Full pipeline respects checkboxes
    inputs = [str(var_chain.get())]
    inputs.append("n")  # rtl_sdr always skipped in pipeline
    inputs.append("y" if var_rtlchannel.get() else "n")
    inputs.append("y" if var_pulsar_det.get() else "n")
    # pul_plot only sent if pulsar_det_an is also selected (loop doesn't break early)
    if var_pulsar_det.get():
        inputs.append("y" if var_pul_plot.get() else "n")

    if var_rtlchannel.get():
        inputs.append(file2_entry.get())   # in_file
        inputs.append(file3_entry.get())   # out_file
        for e in params2_entries:
            inputs.append(e.get())         # clock_rate, OP_data_set_rate, fft_points

    if var_pulsar_det.get():
        # TopoBary inputs (prompted inside pulsar_det_an case)
        inputs.extend(get_topo_inputs())
        # pulsar_det_an inputs (in_file auto-set from RTLChannel4bin if it ran)
        if not var_rtlchannel.get():
            inputs.append(file1_entry.get())
        for i, e in enumerate(params1_entries):
            if i == 4:
                # "ATNF pulsar period [ms]" — NOT prompted by io.py; TopoBary
                # computes it at runtime and passes it directly to pulsar_det_an.
                # Sending it here would shift every subsequent argument by one.
                continue
            inputs.append(e.get())

    captured_output = run_process(inputs, "full pipeline")

    # After the pipeline completes, update the ATNF period field with whatever
    # TopoBary computed, so the GUI reflects what was actually used.
    if var_pulsar_det.get() and captured_output:
        import re
        match = re.search(r"Topocentric period:\s*([\d.eE+\-]+)\s*ms", captured_output)
        if match:
            tc_ms = float(match.group(1))
            period_entry = params1_entries[4]
            period_entry.delete(0, tk.END)
            period_entry.insert(0, f"{tc_ms:.6f}")
            append_output(f"ℹ️  ATNF period field updated to {tc_ms:.6f} ms (TopoBary result)\n")

def run_rtlchannel_only():
    inputs = [str(var_chain.get()), "n", "y", "n"]
    inputs.append(file2_entry.get())
    inputs.append(file3_entry.get())
    for e in params2_entries:
        inputs.append(e.get())
    run_process(inputs, "RTLChannel4bin")

def run_pulsar_det_only():
    inputs = [str(var_chain.get()), "n", "n", "y", "n"]
    inputs.extend(get_topo_inputs())
    inputs.append(file1_entry.get())
    for i, e in enumerate(params1_entries):
        if i == 4:
            # ATNF pulsar period — computed by TopoBary, not prompted by io.py
            continue
        inputs.append(e.get())
    run_process(inputs, "pulsar_det_an")

def run_rtl_sdr_only():
    inputs = [str(var_chain.get()), "y"]
    run_process(inputs, "rtl_sdr")

def run_topobary_only():
    """Run TopoBary with current GUI values and show the result, updating the period field."""
    import subprocess, sys, re
    append_output("\n🚀 Running TopoBary...\n")
    obs_time  = topo_time_entry.get()
    latitude  = topo_lat_entry.get()
    longitude = topo_lon_entry.get()
    p0  = topo_p0_entry.get()
    p1  = topo_p1_entry.get()
    pep = topo_pep_entry.get()

    # Save ATNF params to config so runners.py also picks them up
    config_path = os.path.join(LIB_DIR, "topobary_config.txt")
    with open(config_path, "w") as f:
        f.write(f"P0={p0}\nP1={p1}\nPEP={pep}\n")
    script = f"""
from astropy.time import Time
from astropy.coordinates import SkyCoord, EarthLocation
from astropy import units as u
intm = "{obs_time}"
inlt = {latitude}
inln = {longitude}
Date = Time(intm, format='isot', scale='utc')
mj   = Time(Date, format='mjd')
Loc  = EarthLocation.from_geodetic(lat=inlt, lon=inln, height=0)
sc = SkyCoord(ra=3.54972*u.hr, dec=54.5786*u.deg)
barycorrn = sc.radial_velocity_correction(obstime=Time(mj, format='mjd'), location=Loc)
c = 299792458
P0  = {p0}
P1  = {p1}
PEP = {pep}
PC = ((mj.value - PEP) * P1 * 24 * 3600) + P0
correctionn = barycorrn.value / c * -1e6
TC = PC * (1 + correctionn / 1e6)
print("Doppler Vel =", barycorrn.value, "m/s")
print("ppm =", correctionn)
print("B0329 Barycentric period =", PC)
print("B0329 Topocentric period =", TC)
"""
    try:
        result = subprocess.run(
            [sys.executable, "-c", script],
            capture_output=True, text=True)
        append_output(result.stdout)
        if result.stderr:
            append_output(result.stderr)
        # Parse topocentric period and update the ATNF period field
        match = re.search(r"B0329 Topocentric period\s*=\s*([\d.eE+\-]+)", result.stdout)
        if match:
            tc_ms = float(match.group(1)) * 1000.0
            # Find the ATNF period entry (index 4 in params1_entries)
            period_entry = params1_entries[4]
            period_entry.delete(0, tk.END)
            period_entry.insert(0, f"{tc_ms:.6f}")
            append_output(f"✅ Topocentric period {tc_ms:.6f} ms written to ATNF pulsar period field.\n")
        else:
            append_output("⚠️ Could not parse topocentric period.\n")
    except Exception as e:
        append_output(f"❌ Error: {e}\n")

def run_pul_plot_only():
    # Call pul_plot.py directly — it needs no inputs, just reads its output files
    # from data/pulsar_det_an_results/ and saves PNGs to data/pul_plot_results/
    append_output("\n🚀 Running pul_plot...\n")
    try:
        script_dir = ROOT_DIR
        pul_plot_path = os.path.join(script_dir, "RTL", "src", "pul_plot.py")
        results_dir   = os.path.join(script_dir, "results", "pulsar_det_an_results")
        pul_plot_results_dir = os.path.join(script_dir, "results", "pul_plot_results")
        os.makedirs(pul_plot_results_dir, exist_ok=True)

        process = subprocess.Popen(
            ["python", pul_plot_path, "--no-show"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=results_dir
        )
        output, error = process.communicate()
        append_output(output)
        if error:
            append_output(error)

        # Move PNGs to pul_plot_results
        import shutil
        for fname in ("fig.png", "pixel_plot.png", "pixel2_plot.png", "pixel3_plot.png",
                      "concat_v.png", "concat2_v.png", "concat3_v.png"):
            src = os.path.join(results_dir, fname)
            if os.path.exists(src):
                shutil.move(src, os.path.join(pul_plot_results_dir, fname))
        append_output(f"Plots saved to: {pul_plot_results_dir}\n")
        append_output("✅ Done\n")
    except Exception as e:
        append_output(f"❌ Error: {e}")


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
# TITLE + README BUTTON
# =============================
title_frame = tk.Frame(main)
title_frame.pack(fill="x", pady=(0, 5))

tk.Label(title_frame, text="RAG Višnjan Pulsar Detection Pipeline v1.0.",
         font=("TkDefaultFont", 16, "bold")).pack(side="left")
tk.Label(title_frame, text="a GUI for running Peter East's pulsar analysis software",
         font=("TkDefaultFont", 9), fg="#666666").pack(
    side="left", padx=(12, 0), anchor="s", pady=(0, 3))
tk.Button(title_frame, text="📖 README", command=show_readme,
          font=("TkDefaultFont", 9)).pack(side="right", padx=4)

tk.Frame(main, height=1, bg="#cccccc").pack(fill="x", pady=(0, 8))


# =============================
# PROGRAM CHAIN SELECTION
# =============================
chain_frame = tk.LabelFrame(main, text="Program Chain", padx=10, pady=10)
chain_frame.pack(fill="x", pady=5)

chain_header = tk.Frame(chain_frame)
chain_header.grid(row=0, column=0, columnspan=2, sticky="ew")
make_section_info_button(chain_header, "Program Chain", side="left")

var_chain = tk.IntVar(value=1)

tk.Radiobutton(chain_frame, text="Chain 1  (RTLChannel4bin → pulsar_det_an → pul_plot)",
               variable=var_chain, value=1).grid(row=1, column=0, sticky="w", padx=8)
tk.Radiobutton(chain_frame, text="Chain 2  (not yet implemented)",
               variable=var_chain, value=2, state="disabled").grid(row=2, column=0, sticky="w", padx=8)


# =============================
# PROGRAM SELECTION (checkboxes)
# =============================
sel_frame = tk.LabelFrame(main, text="Program Selection", padx=10, pady=10)
sel_frame.pack(fill="x", pady=5)

sel_header = tk.Frame(sel_frame)
sel_header.grid(row=0, column=0, columnspan=5, sticky="ew")
make_section_info_button(sel_header, "Program Selection", side="left")
tk.Label(sel_header, text="Valid chains:  1→2→3  |  2→3  |  1 only  |  2 only  |  3 only",
         font=("TkDefaultFont", 8), fg="#666666").pack(side="left", padx=(6, 0))

var_rtlchannel = tk.BooleanVar(value=False)
var_pulsar_det = tk.BooleanVar(value=True)
var_pul_plot   = tk.BooleanVar(value=True)

programs = [
    ("RTLChannel4bin", var_rtlchannel),
    ("pulsar_det_an",  var_pulsar_det),
    ("pul_plot",       var_pul_plot),
]
for col, (label, var) in enumerate(programs):
    tk.Checkbutton(sel_frame, text=label, variable=var).grid(row=1, column=col, padx=12, sticky="w")


# =============================
# rtl_sdr  (display only)
# =============================
rtlsdr_frame = tk.LabelFrame(main, text="rtl_sdr", padx=10, pady=10)
rtlsdr_frame.pack(fill="x", pady=5)

make_section_info_button(rtlsdr_frame, "rtl_sdr", anchor="w")

rtlsdr_desc = (
    "Interfaces with the RTL-SDR USB dongle to capture raw IQ samples from the antenna. Not yet implemented so refrain from clicking the run button - nothing will happen.\n\n"
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
topo_frame.columnconfigure(1, weight=1)

make_section_info_button_grid(topo_frame, "TopoBary", row=0, col=0, padx=(0, 8), sticky="w")

tk.Label(topo_frame, text="Observation time - UTC").grid(row=1, column=0, sticky="w")
topo_time_entry = tk.Entry(topo_frame)
topo_time_entry.insert(0, "2025-10-03T04:14:00.0")
topo_time_entry.grid(row=1, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Observation time", row=1, col=2)

tk.Label(topo_frame, text="Latitude - degrees").grid(row=2, column=0, sticky="w")
topo_lat_entry = tk.Entry(topo_frame)
topo_lat_entry.insert(0, "45.29107")
topo_lat_entry.grid(row=2, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Latitude", row=2, col=2)

tk.Label(topo_frame, text="Longitude - degrees").grid(row=3, column=0, sticky="w")
topo_lon_entry = tk.Entry(topo_frame)
topo_lon_entry.insert(0, "13.74837")
topo_lon_entry.grid(row=3, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — Longitude", row=3, col=2)

tk.Frame(topo_frame, height=1, bg="#cccccc").grid(
    row=4, column=0, columnspan=3, sticky="ew", pady=(8, 4))

tk.Label(topo_frame, text="ATNF P0 — ref. period [s]").grid(row=5, column=0, sticky="w")
topo_p0_entry = tk.Entry(topo_frame)
topo_p0_entry.insert(0, "0.714519699726")
topo_p0_entry.grid(row=5, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — P0 reference period [s]", row=5, col=2)

tk.Label(topo_frame, text="ATNF P1 — period derivative [s/s]").grid(row=6, column=0, sticky="w")
topo_p1_entry = tk.Entry(topo_frame)
topo_p1_entry.insert(0, "2.048265e-15")
topo_p1_entry.grid(row=6, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — P1 period derivative [s/s]", row=6, col=2)

tk.Label(topo_frame, text="ATNF PEP — period epoch [MJD]").grid(row=7, column=0, sticky="w")
topo_pep_entry = tk.Entry(topo_frame)
topo_pep_entry.insert(0, "46473.00")
topo_pep_entry.grid(row=7, column=1, sticky="ew")
make_info_button(topo_frame, "TopoBary — PEP period epoch [MJD]", row=7, col=2)

tk.Button(topo_frame, text="▶ Run TopoBary", command=run_topobary_only,
          fg="white", bg="#2255aa").grid(
    row=8, column=0, columnspan=3, pady=(8, 2), sticky="ew")
tk.Label(topo_frame, text="→ updates ATNF pulsar period field in pulsar_det_an",
         font=("TkDefaultFont", 8), fg="#666666").grid(
    row=9, column=0, columnspan=3, sticky="w")

topo_frame.columnconfigure(1, weight=1)


# =============================
# 3-COLUMN ROW: RTLChannel4bin | pulsar_det_an | pul_plot
#
# Strategy for equal height + aligned Run buttons:
#   - cols_frame uses grid with rowconfigure weight=1 so it expands
#   - each LabelFrame gets sticky="nsew" to fill its cell
#   - inside each LabelFrame, a spacer row (weight=1) pushes the Run
#     button to the very last row, so all three buttons line up
# =============================
cols_frame = tk.Frame(main)
cols_frame.pack(fill="both", expand=True, pady=5)
cols_frame.columnconfigure(0, weight=1, uniform="col")
cols_frame.columnconfigure(1, weight=1, uniform="col")
cols_frame.columnconfigure(2, weight=1, uniform="col")
cols_frame.rowconfigure(0, weight=1)


# ---- Column 0: RTLChannel4bin ----
frame2 = tk.LabelFrame(cols_frame, text="RTLChannel4bin", padx=8, pady=8)
frame2.grid(row=0, column=0, sticky="nsew", padx=(0, 4))
frame2.columnconfigure(1, weight=1)

make_section_info_button_grid(frame2, "RTLChannel4bin", row=0, col=0,
                               columnspan=4, sticky="w", pady=(0, 4))

tk.Label(frame2, text="Input file").grid(row=1, column=0, sticky="w")
file2_entry = tk.Entry(frame2)
file2_entry.grid(row=1, column=1, sticky="ew")
tk.Button(frame2, text="…", width=2,
          command=lambda: browse_file(file2_entry)).grid(row=1, column=2)
make_info_button(frame2, "RTLChannel4bin — Input file", row=1, col=3)

tk.Label(frame2, text="Output file").grid(row=2, column=0, sticky="w")
file3_entry = tk.Entry(frame2)
file3_entry.grid(row=2, column=1, sticky="ew")
tk.Button(frame2, text="…", width=2,
          command=lambda: save_file(file3_entry)).grid(row=2, column=2)
make_info_button(frame2, "RTLChannel4bin — Output file", row=2, col=3)

params2_info = [
    ("Sample Rate [MSPS]",     "2.4"),
    ("Data set rate [kHz]",    "1"),
    ("Number of Channels [/]", "16"),
]
params2_entries = []
for i, (name, default) in enumerate(params2_info):
    tk.Label(frame2, text=name).grid(row=3+i, column=0, sticky="w")
    e = tk.Entry(frame2)
    e.insert(0, default)
    e.grid(row=3+i, column=1, sticky="ew")
    params2_entries.append(e)
    make_info_button(frame2, f"RTLChannel4bin — {name}", row=3+i, col=3)

frame2.rowconfigure(3+len(params2_info), weight=1)
tk.Button(frame2, text="▶ Run RTLChannel4bin", command=run_rtlchannel_only,
          fg="white", bg="#2255aa").grid(
    row=4+len(params2_info), column=0, columnspan=4, pady=(8, 2), sticky="sew")


# ---- Column 1: pulsar_det_an ----
frame1 = tk.LabelFrame(cols_frame, text="pulsar_det_an", padx=8, pady=8)
frame1.grid(row=0, column=1, sticky="nsew", padx=2)
frame1.columnconfigure(1, weight=1)

make_section_info_button_grid(frame1, "pulsar_det_an", row=0, col=0,
                               columnspan=4, sticky="w", pady=(0, 4))

tk.Label(frame1, text="Input file").grid(row=1, column=0, sticky="w")
file1_entry = tk.Entry(frame1)
file1_entry.grid(row=1, column=1, sticky="ew")
tk.Button(frame1, text="…", width=2,
          command=lambda: browse_file(file1_entry)).grid(row=1, column=2)
make_info_button(frame1, "pulsar_det_an — Input file", row=1, col=3)

params1_info = [
    ("Number of FFT points",                "16"),
    ("Number of fold sections (Bins)",        "128"),
    ("Number of FFT bins (Window Size)",      "1024"),
    ("Data clock [ms]",             "1"),
    ("ATNF pulsar period [ms]",     "714.4"),   # overridden by TopoBary at runtime
    ("ATNF pulse width [ms]",            "6.5"),
    ("ATNF DM [pc/cm³]",                 "26.7"),
    ("ppm offset [ppm]",                  "-1.3"),
    ("ppm range factor [?]",            "6"),
    ("Threshold sigma [?]",             "1"),
    ("RF bandwidth [MHz]",          "2.4"),
    ("RF centre frequency [MHz]",   "422"),
    ("Roll average [?]",                "50"),
    ("Start section [/]",               "0"),
    ("End section [/]",                 "17"),
]
params1_entries = []
for i, (name, default) in enumerate(params1_info):
    tk.Label(frame1, text=name).grid(row=2+i, column=0, sticky="w")
    e = tk.Entry(frame1)
    e.insert(0, default)
    e.grid(row=2+i, column=1, sticky="ew")
    params1_entries.append(e)
    make_info_button(frame1, f"pulsar_det_an — {name}", row=2+i, col=3)

tk.Button(frame1, text="▶ Run pulsar_det_an", command=run_pulsar_det_only,
          fg="white", bg="#2255aa").grid(
    row=2+len(params1_info), column=0, columnspan=4, pady=(8, 2), sticky="ew")


# ---- Column 2: pul_plot (display only) ----
pulplot_frame = tk.LabelFrame(cols_frame, text="pul_plot", padx=8, pady=8)
pulplot_frame.grid(row=0, column=2, sticky="nsew", padx=(4, 0))
pulplot_frame.columnconfigure(0, weight=1)
pulplot_frame.rowconfigure(1, weight=1)

make_section_info_button_grid(pulplot_frame, "pul_plot", row=0, col=0,
                               sticky="w", pady=(0, 4))

pulplot_desc = (
    "Visualises the folded pulse profile and diagnostic plots produced by pulsar_det_an.\n\n"
    "Input →  Folded profile data file output by pulsar_det_an\n"
    "Output → Matplotlib figures: pulse profile, S/N vs DM, time-phase plot\n\n"
    "This is the final stage in the pipeline. It produces publication-quality plots of the "
    "detected pulse, dispersion measure curve, and frequency-time waterfall for inspection "
    "and verification of the detection."
)
pulplot_text = tk.Text(pulplot_frame, height=7, wrap="word",
                       font=("TkDefaultFont", 9),
                       relief="flat", borderwidth=0,
                       bg=pulplot_frame.cget("bg"))
pulplot_text.insert("1.0", pulplot_desc)
pulplot_text.config(state="disabled")
pulplot_text.grid(row=1, column=0, sticky="nsew", pady=(0, 4))

tk.Button(pulplot_frame, text="▶ Run pul_plot", command=run_pul_plot_only,
          fg="white", bg="#2255aa").grid(row=2, column=0, sticky="sew",
                                         pady=(8, 2), ipadx=4)


# =============================
# RUN FULL PIPELINE + CLEAR OUTPUTS
# =============================
pipeline_row = tk.Frame(main)
pipeline_row.pack(fill="x", pady=(10, 2))
pipeline_row.columnconfigure(0, weight=1)
pipeline_row.columnconfigure(1, weight=1)

tk.Button(pipeline_row, text="🚀 Run Full Pipeline", command=run_pipeline,
          font=("TkDefaultFont", 11, "bold"), pady=6,
          fg="white", bg="#115500").grid(row=0, column=0, sticky="ew", padx=(0, 2))

def clear_all_outputs():
    import shutil
    script_dir = ROOT_DIR
    dirs = [
        os.path.join(script_dir, "results", "pul_plot_results"),
        os.path.join(script_dir, "results", "pulsar_det_an_results"),
    ]
    cleared = []
    for d in dirs:
        d = os.path.normpath(d)
        if os.path.exists(d):
            for f in os.listdir(d):
                fpath = os.path.join(d, f)
                try:
                    if os.path.isfile(fpath) or os.path.islink(fpath):
                        os.remove(fpath)
                    elif os.path.isdir(fpath):
                        shutil.rmtree(fpath)
                except Exception as e:
                    append_output(f"⚠ Could not delete {fpath}: {e}")
            cleared.append(os.path.basename(d))
    if cleared:
        append_output(f"🗑 Cleared: {', '.join(cleared)}\n")
    else:
        append_output("⚠ Output folders not found — nothing cleared.\n")

tk.Button(pipeline_row, text="🗑 Clear All Program Outputs", command=clear_all_outputs,
          font=("TkDefaultFont", 11, "bold"), pady=6,
          fg="white", bg="#882200").grid(row=0, column=1, sticky="ew", padx=(2, 0))

tk.Button(main, text="🗑 Clear Output Log",
          command=lambda: output_box.delete("1.0", tk.END),
          pady=4).pack(fill="x", pady=(2, 10))


# =============================
# OUTPUT
# =============================
output_frame = tk.LabelFrame(main, text="Output", padx=5, pady=5)
output_frame.pack(fill="both", expand=True)

output_box = scrolledtext.ScrolledText(output_frame, height=12)
output_box.pack(fill="both", expand=True)

root.mainloop()
