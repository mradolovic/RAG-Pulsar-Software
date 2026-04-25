import tkinter as tk
from tkinter import filedialog, scrolledtext
import subprocess
import os

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
# RUN MAIN PROGRAM
# -----------------------------
def run_pipeline():
    append_output("\n🚀 Running pipeline...\n")

    inputs = []

    # -----------------------------
    # 1. SELECT PROGRAM CHAIN
    # -----------------------------
    inputs.append("1")   # always chain 1

    # -----------------------------
    # 2. PROGRAM SELECTION (confirm)
    # rtl_sdr = no
    # RTLChannel4bin = yes
    # pulsar_det_an = yes
    # pul_plot = no
    # -----------------------------
    inputs.extend([
        "n",  # rtl_sdr
        "y",  # RTLChannel4bin
        "y",  # pulsar_det_an
        "n",  # pul_plot
    ])

    # -----------------------------
    # 3. RTLChannel4bin inputs
    # -----------------------------
    inputs.append(file2_entry.get())   # input file
    inputs.append(file3_entry.get())   # output file

    for e in params2_entries:
        inputs.append(e.get())

    # -----------------------------
    # 4. TopoBary inputs
    # -----------------------------
    inputs.append(topo_time_entry.get())
    inputs.append(topo_lat_entry.get())
    inputs.append(topo_lon_entry.get())

    # -----------------------------
    # 5. pulsar_det_an inputs
    # -----------------------------
    for e in params1_entries:
        inputs.append(e.get())

    # -----------------------------
    # RUN PROCESS
    # -----------------------------
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
        append_output(error)
        append_output("✅ Done\n")

    except Exception as e:
        append_output(f"❌ Error: {e}")


# -----------------------------
# UI
# -----------------------------
root = tk.Tk()
root.title("Pulsar GUI Runner")
root.geometry("950x750")

main = tk.Frame(root, padx=10, pady=10)
main.pack(fill="both", expand=True)

# =============================
# RTLChannel4bin (TOP)
# =============================
frame2 = tk.LabelFrame(main, text="RTLChannel4bin", padx=10, pady=10)
frame2.pack(fill="x", pady=5)

tk.Label(frame2, text="Input file").grid(row=0, column=0, sticky="w")
file2_entry = tk.Entry(frame2)
file2_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame2, text="Browse", command=lambda: browse_file(file2_entry)).grid(row=0, column=2)

tk.Label(frame2, text="Output file").grid(row=1, column=0, sticky="w")
file3_entry = tk.Entry(frame2)
file3_entry.grid(row=1, column=1, sticky="ew")
tk.Button(frame2, text="Save As", command=lambda: save_file(file3_entry)).grid(row=1, column=2)

params2_info = [
    ("Sample Rate [MSPS]", "2.4"),
    ("Data set rate [kHz]", "1"),
    ("Number of Channels [/]", "16"),
]

params2_entries = []

for i, (name, default) in enumerate(params2_info):
    tk.Label(frame2, text=name).grid(row=2+i, column=0, sticky="w")
    e = tk.Entry(frame2)
    e.insert(0, default)
    e.grid(row=2+i, column=1, sticky="ew")
    params2_entries.append(e)

frame2.columnconfigure(1, weight=1)

# =============================
# TOPOBARY INPUTS
# =============================
topo_frame = tk.LabelFrame(main, text="TopoBary", padx=10, pady=10)
topo_frame.pack(fill="x", pady=5)

tk.Label(topo_frame, text="Observation time").grid(row=0, column=0, sticky="w")
topo_time_entry = tk.Entry(topo_frame)
topo_time_entry.insert(0, "2018-06-27T02:52:00.0")
topo_time_entry.grid(row=0, column=1, sticky="ew")

tk.Label(topo_frame, text="Latitude").grid(row=1, column=0, sticky="w")
topo_lat_entry = tk.Entry(topo_frame)
topo_lat_entry.insert(0, "54.21")
topo_lat_entry.grid(row=1, column=1, sticky="ew")

tk.Label(topo_frame, text="Longitude").grid(row=2, column=0, sticky="w")
topo_lon_entry = tk.Entry(topo_frame)
topo_lon_entry.insert(0, "-1.33")
topo_lon_entry.grid(row=2, column=1, sticky="ew")

topo_frame.columnconfigure(1, weight=1)

# =============================
# pulsar_det_an (BOTTOM)
# =============================
frame1 = tk.LabelFrame(main, text="pulsar_det_an", padx=10, pady=10)
frame1.pack(fill="x", pady=5)

tk.Label(frame1, text="Input file").grid(row=0, column=0, sticky="w")
file1_entry = tk.Entry(frame1)
file1_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame1, text="Browse", command=lambda: browse_file(file1_entry)).grid(row=0, column=2)

params1_info = [
    ("Param 1", "16"),
    ("Param 2", "1"),
    ("ATNF pulsar period [ms]", "714.47415"),
    ("Bins", "128"),
    ("Window Size", "1024"),
    ("Threshold", "6.5"),
    ("ATNF DM [pc/cm^3]", "-26.7"),
    ("ppm offset", "-1.3"),
    ("Factor", "6"),
    ("Mode", "1"),
    ("RF bandwidth", "2.4"),
    ("RF centre frequency", "422"),
    ("ppm range factor", "50"),
    ("Start bin", "0"),
    ("End bin", "127"),
]

params1_entries = []

for i, (name, default) in enumerate(params1_info):
    tk.Label(frame1, text=name).grid(row=1+i, column=0, sticky="w")
    e = tk.Entry(frame1)
    e.insert(0, default)
    e.grid(row=1+i, column=1, sticky="ew")
    params1_entries.append(e)

frame1.columnconfigure(1, weight=1)

# =============================
# RUN BUTTON
# =============================
tk.Button(main, text="🚀 Run Full Pipeline", command=run_pipeline).pack(pady=10)

# =============================
# OUTPUT
# =============================
output_frame = tk.LabelFrame(main, text="Output", padx=5, pady=5)
output_frame.pack(fill="both", expand=True)

output_box = scrolledtext.ScrolledText(output_frame)
output_box.pack(fill="both", expand=True)

# -----------------------------
root.mainloop()
