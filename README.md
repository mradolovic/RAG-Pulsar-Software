# RAG Višnjan Amateur Pulsar Detection Pipeline v1.1.

A GUI front-end for pulsar_det_an - Peter East's software alternative to PRESTO.

Developed in 2026 by:

Peter East          - pulsar detection software \
Martin Ante Rogošić - parsing script and compiling \
Marko Radolović     - GUI


# Overview


This software provides a graphical interface for running a chain of programs
that detect and analyse pulsar signals from RTL-SDR radio observations. It is
designed for amateur radio astronomers who want to detect real pulsars — such as
B0329+54 — using affordable hardware and open-source software.

The pipeline takes raw IQ data from an RTL-SDR dongle, converts it into
frequency-channelled data, searches it for a periodic signal at the expected
pulsar period and dispersion measure, and produces a set of diagnostic plots.
Before the detection stage, a barycentric-to-topocentric period correction is
applied automatically using TopoBary, which accounts for the Doppler shift
caused by the Earth's motion relative to the pulsar.


# Pipeline stages


  1. rtl_sdr
     Captures raw 8-bit IQ samples from the RTL-SDR USB dongle and saves them
     to a binary .bin file. Not yet implemented in this GUI — use the rtl_sdr
     command-line tool directly to record your observation, then point the GUI
     at the resulting file.

  2. RTLChannel4bin
     Reads the raw .bin file and converts it into a channelised 4-byte float
     binary file. It applies an N-point FFT to split the receiver bandwidth into
     frequency channels, then time-averages groups of FFT frames to reduce the
     data rate to a manageable output rate. The result is a compact binary file
     where each row contains the power in each frequency channel at one time
     step. This file is the input to pulsar_det_an.

  3. TopoBary  (runs automatically before pulsar_det_an)
     Computes the topocentric pulse period for the target pulsar at your
     observing site and time. It uses the ATNF catalogue parameters (P0, P1,
     epoch) to find the current barycentric period, then applies a Doppler
     correction based on the Earth's radial velocity toward the pulsar. The
     resulting topocentric period is used as the centre of the period search in
     pulsar_det_an. You can also run TopoBary independently using the Run
     TopoBary button, which will update the ATNF period field in the GUI.

  4. pulsar_det_an
     The core detection and analysis program. It reads the channelised binary
     file, folds the data at a trial period, and searches over a range of
     dispersion measures, periods, and period derivatives (p-dot) to find the
     combination that gives the highest signal-to-noise ratio. A Gaussian
     matched filter shaped to the expected pulse width is used to improve
     sensitivity. Results are written to a set of text files covering the folded
     pulse profile, DM search, period search, p-dot search, band SNR, section
     SNR timeline, rolling average, and waterfalls.

  5. pul_plot
     Reads all the text output files from pulsar_det_an and renders them as a
     set of diagnostic plots. These include the folded pulse profile, DM search
     curve, period and p-dot search, band SNR chart, cumulative SNR growth,
     rolling average SNR, compressed data waterfall, and a 2D period/p-dot
     image. All plots are saved as PNG files and stitched into a single combined
     image for easy review.

---

# Features

## Signal Processing

- C-based processing binaries for performance-critical operations
- FFT and mathematical processing support
- Chained program execution pipelines
- Automated compilation helpers

## Astronomy Utilities

Using `astropy` for:

- Coordinate transformations
- Earth-based observer positioning
- Time conversions
- Sky coordinate calculations

## Visualization

- Real-time and static plotting with Matplotlib
- Numerical analysis with NumPy

## GUI Interface

Tkinter-based desktop interface with:

- File selection dialogs
- Scrollable console output
- Interactive execution controls
- Processing chain management

---

# Project Structure - ToDo!

```text
RAG-Pulsar-Software/
├── RTL/                  # C source code and Makefiles
│   ├── src/
│   ├── bin/
│   └── Makefile
├── lib/                  # Python helper modules
│   ├── funcs.py
│   ├── io.py
│   └── runners.py
├── *.py                  # Main application scripts
├── requirements.txt
└── README.md
```

---

# Requirements

## System Dependencies

Install required system packages:

Ubuntu

```bash
sudo apt install python3 python3-pip python3-tk build-essential
```

Arch Linux

```bash
sudo pacman -S python python-pip tk base-devel
```

This installs:

- Python
- pip
- Tk GUI toolkit (`tkinter` backend)
- GCC / Make / linker toolchain

---

# Python Dependencies

Install dependencies:

Ubuntu:

```bash
pip install -r requirements.txt
```

Arch Linux:

- install packages from requirements.txt by hand. Beware of (possible) different package names with Pacman.

---

# Installation

## 1. Clone the Repository

```bash
git clone <your-repo-url>
cd RAG-Pulsar-Software
```

## 2. Create a Virtual Environment

If you want to run the program in a virtual enviroment, you can do so with Python's 'venv'

## 3. Install Python Dependencies

```bash
pip install -r requirements.txt
```

## 4. Build the C Utilities

The GUI program (gui.py) automatically compiles all of the necessary C programs. If they are already compiled, the compile command is disregarded.

If one wants to compile the programs by hand, which is unnecessary, one can do so in the following way:

```bash
cd RTL
make all
```

The makefile contains the compilation commands for all necessary C programs.

Generated binaries should appear in:

```text
RTL/bin/
```

---

# Running the Software

Launch the main Python application:

```bash
python gui.py
```

---
# Example Workflow

1. Record data from an SDR in IQ format
2. Channelise the data using RTLChannel4bin
3. Perform astronomical coordinate transformation with Astropy
4. Analyse the data using pulsar_det_an
5. Visualize results using Matplotlib
---

# Troubleshooting

## `ModuleNotFoundError`

Activate your virtual environment:

```bash
source venv/bin/activate
```

Then reinstall dependencies:

```bash
pip install -r requirements.txt
```

---

## `tkinter` Errors

On Arch Linux ensure Tk is installed:

```bash
sudo pacman -S tk
```

---

## GCC / Make Missing

Install the development toolchain:

```bash
sudo pacman -S base-devel
```

---

# Contributing

Contributions, bug reports, and improvements are welcome.

Suggested areas:

- Signal processing optimizations
- Additional astronomy tooling
- GUI improvements
- Data pipeline enhancements
- Documentation

<<<<<<< HEAD
---
=======
---
>>>>>>> 5140bad (change big readme)
