# RAG Pulsar Software

THE README IS NOT YES FINISHED, IT IS AI GENERATED SO BEWARE OF STUPID INSTRUCTIONS. This is just serves me as a Lorem Ipsum readme

A modular radio astronomy and pulsar signal processing toolkit written in Python and C.

This project combines:

- High-performance C signal processing utilities
- Python-based visualization tools
- Astronomical coordinate/time calculations
- Interactive GUI tooling using Tkinter
- Plotting and image-processing pipelines

The software is designed for experimentation, analysis, and processing workflows involving pulsar and RTL-SDR related data.

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
- Image handling with Pillow
- Numerical analysis with NumPy

## GUI Interface

Tkinter-based desktop interface with:

- File selection dialogs
- Scrollable console output
- Interactive execution controls
- Processing chain management

---

# Project Structure

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

## System Dependencies (Arch Linux)

Install required system packages:

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

Create a `requirements.txt` file:

```txt
numpy==2.4.4
matplotlib==3.10.9
Pillow==12.2.0
astropy==7.2.0
```

Install dependencies:

```bash
pip install -r requirements.txt
```

---

# Installation

## 1. Clone the Repository

```bash
git clone <your-repo-url>
cd RAG-Pulsar-Software
```

## 2. Create a Virtual Environment

ToDo

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

# Development Notes

## Python

The project uses:

- `numpy` for numerical processing
- `matplotlib` for plotting
- `Pillow` for image manipulation
- `astropy` for astronomy calculations

## C Components

The `RTL/` directory contains native processing utilities compiled with GCC.

Most binaries link against:

- `libm` (`-lm`)

---

# Example Workflow

1. Acquire SDR or pulsar-related data
2. Process raw data using C utilities
3. Execute processing chains through Python orchestration
4. Visualize results using Matplotlib
5. Perform astronomical coordinate analysis with Astropy

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

# Recommended Development Environment

- OS: Arch Linux
- Python: 3.11+
- GCC: Latest stable
- Virtual environments strongly recommended

---

# Contributing

Contributions, bug reports, and improvements are welcome.

Suggested areas:

- Signal processing optimizations
- Additional astronomy tooling
- GUI improvements
- Data pipeline enhancements
- Documentation

---

# License

Add your preferred license here.

Examples:

- MIT
- GPLv3
- Apache 2.0

---

# Acknowledgements

Built using:

- Python
- GCC
- NumPy
- Matplotlib
- Astropy
- Pillow
- Tkinter

