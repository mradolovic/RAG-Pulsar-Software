import tkinter as tk
from tkinter import filedialog, scrolledtext
import subprocess
import time
import os

# -----------------------------
# Helpers
# -----------------------------
def run_command(cmd):
    start = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.time() - start
    return result, elapsed


def append_output(text):
    output_box.insert(tk.END, text + "\n")
    output_box.see(tk.END)


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


def execute(cmd):
    append_output(f"\n▶ {' '.join(cmd)}")
    result, elapsed = run_command(cmd)
    append_output(f"⏱ {elapsed:.2f}s")
    append_output(result.stdout)
    append_output(result.stderr)


# -----------------------------
# Commands
# -----------------------------
def run_second():
    params = [e.get() for e in params2_entries]

    cmd = ["./RTLChannel4bin.out",
           file2_entry.get(),
           file3_entry.get()] + params

    execute(cmd)


def run_first():
    params = [e.get() for e in params1_entries]

    cmd = ["./pulsar_det_an.out",
           file1_entry.get()] + params

    execute(cmd)


def run_pipeline():
    append_output("\n🚀 Running full pipeline...\n")
    run_second()
    run_first()
    append_output("✅ Done\n")


# -----------------------------
# UI
# -----------------------------
root = tk.Tk()
root.title("Pulsar Pipeline")
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

# Parameter names (EDIT THESE to real meanings if you know them)
params2_info = [
    ("Sample Rate", "2.4"),
    ("Mode", "1"),
    ("Channels", "16"),
]

params2_entries = []

for i, (name, default) in enumerate(params2_info):
    tk.Label(frame2, text=name).grid(row=2+i, column=0, sticky="w")
    e = tk.Entry(frame2)
    e.insert(0, default)
    e.grid(row=2+i, column=1, sticky="ew")
    params2_entries.append(e)

tk.Button(frame2, text="Run", command=run_second).grid(row=2+len(params2_info), column=0, columnspan=3, pady=5)

frame2.columnconfigure(1, weight=1)

# =============================
# pulsar_det_an (BOTTOM)
# =============================
frame1 = tk.LabelFrame(main, text="pulsar_det_an", padx=10, pady=10)
frame1.pack(fill="x", pady=5)

tk.Label(frame1, text="Input file").grid(row=0, column=0, sticky="w")
file1_entry = tk.Entry(frame1)
file1_entry.grid(row=0, column=1, sticky="ew")
tk.Button(frame1, text="Browse", command=lambda: browse_file(file1_entry)).grid(row=0, column=2)

# Parameter names (EDIT THESE)
params1_info = [
    ("Param 1", "16"),
    ("Param 2", "1"),
    ("Frequency", "714.47415"),
    ("Bins", "128"),
    ("Window Size", "1024"),
    ("Threshold", "6.5"),
    ("Offset A", "-26.7"),
    ("Offset B", "-1.3"),
    ("Factor", "6"),
    ("Mode", "1"),
    ("Scale", "2.4"),
    ("Index", "422"),
    ("Limit", "50"),
    ("Flag", "0"),
    ("Max Value", "127"),
]

params1_entries = []

for i, (name, default) in enumerate(params1_info):
    tk.Label(frame1, text=name).grid(row=1+i, column=0, sticky="w")
    e = tk.Entry(frame1)
    e.insert(0, default)
    e.grid(row=1+i, column=1, sticky="ew")
    params1_entries.append(e)

tk.Button(frame1, text="Run", command=run_first).grid(row=1+len(params1_info), column=0, columnspan=3, pady=5)

frame1.columnconfigure(1, weight=1)

# =============================
# PIPELINE
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
