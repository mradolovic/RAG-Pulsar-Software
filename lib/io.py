import itertools
import enum
from .funcs import confirm
from pprint import pprint


class FieldKind(enum.Enum):
    SINGLE = "single"  # exactly one value entered
    MULTI  = "multi"   # multiple space-separated values
    RANGE  = "range"   # dash-separated range (start-end-step)


def parse_range(raw, dtype):
    """
    Try to parse a dash-separated range string 'start-end-step'.
    Returns (values, errors) if it looks like a range, or (None, None) if not.
    """
    parts = raw.split("-")
    if len(parts) != 3:
        return None, None

    errors = []
    parsed = []
    for label, part in zip(["start", "end", "step"], parts):
        try:
            parsed.append(float(part))
        except ValueError:
            errors.append(f"  '{part}' is not a valid number for range {label}")

    if errors:
        return [], errors

    start, end, step = parsed
    if step == 0:
        return [], ["  Step cannot be zero"]
    if (end - start) / step < 0:
        return [], ["  Step direction is inconsistent with start and end values"]

    values = []
    current = start
    while current <= end + 1e-9:
        values.append(dtype(current))
        current += step

    return values, []


def parse_field(raw, dtype, can_vary):
    """
    Parse and validate input into a list of dtype and a FieldKind.
    Supports:
      - single value:           "42"
      - space-separated multi:  "1 2 3"    (only if can_vary=True)
      - dash-separated range:   "1-30-2"   (only if can_vary=True)
    Returns (values, kind, errors). On any error, values is [].
    """
    stripped = raw.strip()
    tokens = stripped.split()

    # Try range: exactly one token with exactly two dashes (numeric types only)
    if dtype is not str and len(tokens) == 1 and tokens[0].count("-") == 2:
        values, errors = parse_range(tokens[0], dtype)
        if errors is not None:  # was recognised as a range attempt
            if not can_vary:
                return [], FieldKind.RANGE, ["  This field cannot be varied"]
            if errors:
                return [], FieldKind.RANGE, errors
            kind = FieldKind.SINGLE if len(values) == 1 else FieldKind.RANGE
            return values, kind, []

    # Space-separated parsing
    parts = stripped.split()
    if not parts:
        return [], FieldKind.SINGLE, []

    if not can_vary and len(parts) > 1:
        return [], FieldKind.MULTI, ["  This field cannot be varied"]

    values = []
    errors = []
    for part in parts:
        try:
            values.append(dtype(part))
        except ValueError:
            errors.append(f"  '{part}' is not a valid {dtype.__name__}")

    if errors:
        return [], FieldKind.SINGLE, errors

    kind = FieldKind.SINGLE if len(values) == 1 else FieldKind.MULTI
    return values, kind, []


def prompt_field(prompt, dtype, can_vary=False):
    """
    Prompt the user for a field, re-asking only if validation fails.
    Returns (FieldKind, values).

    If can_vary=False, only a single value is accepted.
    If can_vary=True, space-separated or start-end-step range input is also accepted.
    """
    hint = " (single value only)" if not can_vary else " (space-separated values, or start-end-step range)"
    while True:
        raw = input(f"{prompt}{hint}: ")
        values, kind, errors = parse_field(raw, dtype, can_vary)
        if not errors:
            return kind, values
        print("Invalid input, please fix the following:")
        for e in errors:
            print(e)


def console_program_chain_io():
    program_chain_num = int(input("Do you wish to run program chain 1 or program chain 2: "))
    if program_chain_num not in (1, 2):
        print("For program chain enter either 1 or 2, for more information run StarGazer.py --help or checkout the readMe.txt")
        exit()

    if program_chain_num == 1:
        programs = [
            ("rtl_sdr",        "Do you wish to run rtl_sdr"),
            ("RTLChannel4bin", "Do you wish to run RTLChannel4bin.c?"),
            ("pulsar_det_an",  "Do you wish to run pulsar_det_an.c?"),
            ("pul_plot",       "Do you wish to run pul_plot.py?"),
        ]

        started = False
        selected_names = []
        for name, prompt in programs:
            if name == "rtl_sdr":
                if confirm(prompt, default=False):
                    selected_names.append(name)
                    started = True
            elif confirm(prompt, default=True):
                selected_names.append(name)
                started = True
            elif started:
                break

        program_chain, combinations = console_program_one_io(selected_names)

    elif program_chain_num == 2:
        print("Currently only program chain 1 is supported :( , we are working on it")

    return program_chain, combinations


def console_program_one_io(selected_names):
    """
    Collects parameters for each selected program.
    TopoBary runs automatically alongside pulsar_det_an — its inputs are
    prompted in a clearly labelled section and its output (topo_period_ms)
    is passed to pulsar_det_an automatically.

    Returns:
      - program_chain: list of (program_name, is_varied) tuples.
                       TopoBary is always is_varied=False.
      - combinations:  list of dicts, one per parameter combination.
    """
    res = {}  # field_key -> (FieldKind, [values])

    for program in selected_names:
        match program:
            case "rtl_sdr":
                print("We do not currently have the code for rtl_sdr, we are working on it :(")
                exit()

            case "RTLChannel4bin":
                res["RTLChannel4bin_in_file"]          = prompt_field("Enter the file path to the input data for RTLChannel4bin",    str,   can_vary=False)
                res["RTLChannel4bin_out_file"]         = prompt_field("Enter the file path you want RTLChannel4bin to output to",    str,   can_vary=False)
                res["RTLChannel4bin_clock_rate"]       = prompt_field("Enter the sample rate of your data in MSPS (e.g. 2)",         float, can_vary=False)
                res["RTLChannel4bin_OP_data_set_rate"] = prompt_field("Enter O/P data set rate in kHz (e.g. 1)",                    float, can_vary=False)
                res["RTLChannel4bin_fft_points"]       = prompt_field("Enter the number of FFT points N (e.g. 16)",                 int,   can_vary=False)

            case "pulsar_det_an":
                # --- TopoBary inputs (runs automatically before pulsar_det_an) ---
                print("\n-- TopoBary inputs --")
                res["topo_barry_observation_time"] = prompt_field("Enter the observation time in the following format 2018-06-27T02:52:00.0", str,   can_vary=False)
                res["topo_barry_latitude"]         = prompt_field("Enter the observation latitude in the following format 54.21",             float, can_vary=False)
                res["topo_barry_longitude"]        = prompt_field("Enter the observation longitude in the following format -1.33",            float, can_vary=False)
                # topo_period_ms is captured from TopoBary output at runtime and
                # passed to pulsar_det_an automatically — not prompted here.

                # --- pulsar_det_an inputs ---
                print("\n-- pulsar_det_an inputs --")
                if "RTLChannel4bin_out_file" in res:
                    res["pulsar_det_an_in_file"] = res["RTLChannel4bin_out_file"]
                else:
                    res["pulsar_det_an_in_file"] = prompt_field("Enter the path to the input .bin file", str, can_vary=False)

                res["pulsar_det_an_fft_points"]       = prompt_field("Enter the N-point FFT size (e.g. 16)",                                    int,   can_vary=False)
                res["pulsar_det_an_data_clock_ms"]    = prompt_field("Enter the data clock in ms (e.g. 1)",                                     float, can_vary=False)
                res["pulsar_det_an_fold_sections"]    = prompt_field("Enter the number of fold sections (e.g. 128)",                            int,   can_vary=False)
                res["pulsar_det_an_fft_bins"]         = prompt_field("Enter the number of FFT bins (e.g. 1024)",                                int,   can_vary=False)
                res["pulsar_det_an_pulse_width"]      = prompt_field("Enter the ATNF pulse width in ms (e.g. 6.5)",                             float, can_vary=False)
                res["pulsar_det_an_dm"]               = prompt_field("Enter the ATNF dispersion measure DM in pc/cm^3 (e.g. 26.7)",             float, can_vary=False)
                res["pulsar_det_an_ppm_offset"]       = prompt_field("Enter the ppm offset (e.g. 6)",                                           float, can_vary=True)
                res["pulsar_det_an_ppm_range_factor"] = prompt_field("Enter the ppm range factor (e.g. 2)",                                     float, can_vary=False)
                res["pulsar_det_an_threshold_sigma"]  = prompt_field("Enter the threshold sigma (e.g. 2)",                                      float, can_vary=False)
                res["pulsar_det_an_rf_band_mhz"]      = prompt_field("Enter the RF bandwidth in MHz (e.g. 40)",                                 float, can_vary=False)
                res["pulsar_det_an_rf_center_mhz"]    = prompt_field("Enter the RF center frequency in MHz (e.g. 35)",                          float, can_vary=False)
                res["pulsar_det_an_roll_avg"]         = prompt_field("Enter the roll average number (e.g. 0)",                                  int,   can_vary=False)
                res["pulsar_det_an_start_section"]    = prompt_field("Enter the start section (e.g. 0)",                                        int,   can_vary=False)
                res["pulsar_det_an_end_section"]      = prompt_field("Enter the end section (e.g. 127)",                                        int,   can_vary=False)

            case "pul_plot":
                pass

    # Build program_chain with is_varied flags per program.
    # TopoBary is inserted automatically before pulsar_det_an, always is_varied=False.
    program_fields = {
        "RTLChannel4bin": [k for k in res if k.startswith("RTLChannel4bin")],
        "topo_barry":     [k for k in res if k.startswith("topo_barry")],
        "pulsar_det_an":  [k for k in res if k.startswith("pulsar_det_an")],
        "pul_plot":       [],
    }

    program_chain = []
    for name in selected_names:
        if name == "pulsar_det_an":
            # Insert TopoBary first, always non-varied
            program_chain.append(("topo_barry", False))

        is_varied = any(
            res[k][0] in (FieldKind.MULTI, FieldKind.RANGE)
            for k in program_fields.get(name, [])
            if k in res
        )
        program_chain.append((name, is_varied))

    # Warn user about total combinations
    keys = list(res.keys())
    value_lists = [res[k][1] for k in keys]
    total = 1
    for v in value_lists:
        total *= len(v)
    print(f"\nThis will run {total} combination(s).")
    if total > 100:
        if not confirm(f"That's {total} runs, are you sure?", default=False):
            exit()

    # Build combinations
    combinations = [
        dict(zip(keys, combo))
        for combo in itertools.product(*value_lists)
    ]

    return program_chain, combinations