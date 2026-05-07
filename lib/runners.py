import subprocess
import sys
import os
import re

# runners.py lives in lib/, so go up one level to reach RTL/bin/
BIN_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "RTL", "bin")

# Ensure blank attenuation files exist in BIN_DIR (created on first import)
for _blank in ("Blankf.txt", "Blanks.txt"):
    _blank_path = os.path.join(BIN_DIR, _blank)
    if not os.path.exists(_blank_path):
        open(_blank_path, "w").close()

def _bin(name):
    """Return the platform-appropriate binary path."""
    ext = ".exe" if sys.platform == "win32" else ".out"
    return os.path.join(BIN_DIR, name + ext)


def run_topobary(combination):
    """
    Run TopoBary with the user's observation parameters.
    Captures the topocentric period and returns it in milliseconds.
    """
    obs_time  = combination["topo_barry_observation_time"]
    latitude  = combination["topo_barry_latitude"]
    longitude = combination["topo_barry_longitude"]

    # Load ATNF params from config file written by GUI, fall back to defaults
    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "topobary_config.txt")
    if os.path.exists(config_path):
        cfg = {}
        with open(config_path) as f:
            for line in f:
                line = line.strip()
                if "=" in line:
                    k, v = line.split("=", 1)
                    cfg[k.strip()] = float(v.strip())
        P0  = cfg.get("P0",  0.714519699726)
        P1  = cfg.get("P1",  2.048265e-15)
        PEP = cfg.get("PEP", 46473.00)
    else:
        P0  = 0.714519699726
        P1  = 2.048265e-15
        PEP = 46473.00

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

P0  = {P0}
P1  = {P1}
PEP = {PEP}

PC = ((mj.value - PEP) * P1 * 24 * 3600) + P0
correctionn = barycorrn.value / c * -1e6
TC = PC * (1 + correctionn / 1e6)

print("Doppler Vel =", barycorrn.value, "m/s")
print("ppm =", correctionn)
print("B0329 Barycentric period =", PC)
print("B0329 Topocentric period =", TC)
"""

    print("\n-- Running TopoBary --")
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True, text=True
    )
    print(result.stdout)
    if result.returncode != 0:
        print("TopoBary error:", result.stderr)
        return None

    match = re.search(r"B0329 Topocentric period\s*=\s*([\d.eE+\-]+)", result.stdout)
    if match:
        tc_ms = float(match.group(1)) * 1000.0
        print(f"Topocentric period: {tc_ms:.6f} ms")
        return tc_ms
    else:
        print("WARNING: Could not parse topocentric period from TopoBary output.")
        return None


def run_rtlchannel4bin(combination):
    """
    RTLChannel4bin command:
      rtlchannel4bin <infile> <outfile> <clock_MHz> <ds_kHz> <fft_points>
    """
    cmd = [
        _bin("RTLChannel4bin"),
        combination["RTLChannel4bin_in_file"],
        combination["RTLChannel4bin_out_file"],
        str(combination["RTLChannel4bin_clock_rate"]),
        str(combination["RTLChannel4bin_OP_data_set_rate"]),
        str(combination["RTLChannel4bin_fft_points"]),
    ]
    print("\n-- Running RTLChannel4bin --")
    print("Command:", " ".join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=BIN_DIR)
    print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr)
    if result.returncode != 0:
        print(f"RTLChannel4bin exited with code {result.returncode}")
    return result.returncode == 0


def run_pulsar_det_an(combination, topo_period_ms=None):
    """
    pulsar_det_an command (argument order inferred from C source and io.py):
      pulsar_det_an <infile> <fft_points> <data_clock_ms> <topo_period_ms>
                    <fold_sections> <fft_bins> <pulse_width> <dm>
                    <ppm_offset> <threshold_sigma> <ppm_range_factor>
                    <rf_band_mhz> <rf_center_mhz> <roll_avg>
                    <start_section> <end_section>

    topo_period_ms is read from combination["pulsar_det_an_topo_period_ms"]
    (written by run_topobary via run_program_chain).  The optional argument
    is kept for callers that pass it directly (e.g. run_pulsar_det_only in the GUI).
    """
    # Prefer the value stored in the combination dict; fall back to the argument.
    period_ms = combination.get("pulsar_det_an_topo_period_ms", topo_period_ms)
    if period_ms is None:
        print("ERROR: No topocentric period — TopoBary must succeed before pulsar_det_an.")
        return False

    cmd = [
        _bin("pulsar_det_an_v4"),
        combination["pulsar_det_an_in_file"],              # <data file>
        str(combination["pulsar_det_an_fft_points"]),      # <N-point FFT>
        str(combination["pulsar_det_an_data_clock_ms"]),   # <data clock (ms)>
        str(period_ms),                                    # <pulsar period (ms)> — from TopoBary
        str(combination["pulsar_det_an_fold_sections"]),   # <No: sections>
        str(combination["pulsar_det_an_fft_bins"]),        # <No. bins>
        str(combination["pulsar_det_an_pulse_width"]),     # <pulse width>
        str(combination["pulsar_det_an_dm"]),              # <DM>
        str(combination["pulsar_det_an_ppm_offset"]),      # <ppm>
        str(combination["pulsar_det_an_threshold_sigma"]), # <spike threshold>
        str(combination["pulsar_det_an_ppm_range_factor"]),# <ppm range factor>
        str(combination["pulsar_det_an_rf_band_mhz"]),     # <RF band (MHz)>
        str(combination["pulsar_det_an_rf_center_mhz"]),   # <RF Centre (MHz)>
        str(combination["pulsar_det_an_roll_avg"]),        # <roll average No.>
        str(combination["pulsar_det_an_start_section"]),   # <start section>
        str(combination["pulsar_det_an_end_section"]),     # <end section>
    ]
    # pulsar_det_an writes all output files with relative paths,
    # so we control the output location purely via cwd.
    results_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "results", "pulsar_det_an_results"
    )
    os.makedirs(results_dir, exist_ok=True)

    # Blankf.txt and Blanks.txt must be present in cwd — copy them in if needed
    for blank in ("Blankf.txt", "Blanks.txt"):
        src = os.path.join(BIN_DIR, blank)
        dst = os.path.join(results_dir, blank)
        if os.path.exists(src) and not os.path.exists(dst):
            import shutil
            shutil.copy2(src, dst)

    print("\n-- Running pulsar_det_an --")
    print("Command:", " ".join(cmd))
    print(f"Output directory: {results_dir}")
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=results_dir)
    print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr)
    if result.returncode != 0:
        print(f"pulsar_det_an exited with code {result.returncode}")
    return result.returncode == 0


def run_pul_plot():
    """Run pul_plot.py from the same directory as this script."""
    pul_plot_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "RTL", "src", "pul_plot.py")
    results_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "results", "pulsar_det_an_results"
    )
    pul_plot_results_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "results", "pul_plot_results"
    )
    os.makedirs(pul_plot_results_dir, exist_ok=True)

    print("\n-- Running pul_plot --")
    result = subprocess.run([sys.executable, pul_plot_path, "--no-show"], capture_output=True, text=True, cwd=results_dir)
    print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr)

    # Move PNG outputs to pul_plot_results
    import shutil
    for fname in ("fig.png", "pixel_plot.png", "pixel2_plot.png", "pixel3_plot.png", "concat_v.png", "concat2_v.png", "concat3_v.png"):
        src = os.path.join(results_dir, fname)
        if os.path.exists(src):
            shutil.move(src, os.path.join(pul_plot_results_dir, fname))
    print(f"Plots saved to: {pul_plot_results_dir}")

    return result.returncode == 0


def run_program_chain(program_chain, combinations):
    """
    Execute the full program chain for every parameter combination.
    TopoBary is inserted before pulsar_det_an by io.py and handled here.
    """
    for combination in combinations:
        print(f"\n{'='*60}")
        print(f"Combination: { {k: v for k, v in combination.items()} }")
        print(f"{'='*60}")

        topo_period_ms = None

        for program, is_varied in program_chain:
            match program:
                case "rtl_sdr":
                    print("rtl_sdr is not yet implemented.")

                case "RTLChannel4bin":
                    if not run_rtlchannel4bin(combination):
                        print("RTLChannel4bin failed — stopping this combination.")
                        break

                case "topo_barry":
                    topo_period_ms = run_topobary(combination)
                    if topo_period_ms is None:
                        print("TopoBary failed — stopping this combination.")
                        break

                    # Store as the authoritative period for this combination.
                    # run_pulsar_det_an reads it from here; the GUI runner also
                    # updates the ATNF period field via this value after the chain.
                    combination["pulsar_det_an_topo_period_ms"] = topo_period_ms

                case "pulsar_det_an":
                    if not run_pulsar_det_an(combination, topo_period_ms):
                        print("pulsar_det_an failed — stopping this combination.")
                        break

                case "pul_plot":
                    run_pul_plot()
