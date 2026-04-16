import itertools
from .funcs import confirm
from pprint import pprint
import json
def parse_field(raw, dtype, field_name):
    """
    Parse and validate a space-separated string of values into a list of dtype.
    Returns (values, errors).
    """
    parts = raw.split()  # handles any number of spaces
    values = []
    errors = []
    for part in parts:
        try:
            values.append(dtype(part))
        except ValueError:
            errors.append(f"  '{part}' is not a valid {dtype.__name__}")
    return values, errors


def prompt_field(prompt, dtype):
    """
    Prompt the user for a field, re-asking only if validation fails.
    Returns a list of values of the given dtype.
    """
    while True:
        raw = input(f"{prompt} (space-separate multiple values): ")
        values, errors = parse_field(raw, dtype, prompt)
        if not errors:
            return values
        print(f"Invalid input, please fix the following:")
        for e in errors:
            print(e)


def console_program_chain_io():
    program_chain_num = int(input("Do you wish to run program chain 1 or program chain 2: "))
    program_chain = []
    if program_chain_num != 1 and program_chain_num != 2:
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
        for name, prompt in programs:
            if confirm(prompt, default=True):
                program_chain.append(name)
                started = True
            elif started:
                break
        console_program_one_io(program_chain)
    if program_chain_num == 2:
        print("Currently only program chain 1 is supported :( , we are working on it")

    return program_chain


def console_program_one_io(program_chain):
    res = {}
    for program in program_chain:
        match program:
            case "rtl_sdr":
                print("We do not currently have the code for rtl_sdr, we are working on it :(")
                exit()
            case "RTLChannel4bin":
                res["RTLChannel4bin_in_file"]          = prompt_field("Enter the file path to the input data for RTLChannel4bin", str)
                res["RTLChannel4bin_out_file"]         = prompt_field("Enter the file path you want RTLChannel4bin to output to", str)
                res["RTLChannel4bin_clock_rate"]       = prompt_field("Enter the sample rate of your data in MSPS (e.g. 2)", float)
                res["RTLChannel4bin_OP_data_set_rate"] = prompt_field("Enter O/P data set rate in kHz (e.g. 1)", float)
                res["RTLChannel4bin_fft_points"]       = prompt_field("Enter the number of FFT points N (e.g. 16)", int)
            case "pulsar_det_an":
                res["topo_barry_observation_time"] = prompt_field("Enter the observation time in the following format 2018-06-27T02:52:00.0", str)
                res["topo_barry_latitude"]         = prompt_field("Enter the observation latitude in the following format 54.21", float)
                res["topo_barry_longitude"]        = prompt_field("Enter the observation longitude in the following format -1.33", float)

                if "RTLChannel4bin_out_file" in res:
                    res["pulsar_det_an_in_file"] = res["RTLChannel4bin_out_file"]
                else:
                    res["pulsar_det_an_in_file"] = prompt_field("Enter the path to the input .bin file", str)

                res["pulsar_det_an_fft_points"]       = prompt_field("Enter the N-point FFT size (e.g. 16)", int)
                res["pulsar_det_an_data_clock_ms"]    = prompt_field("Enter the data clock in ms (e.g. 1)", float)
                res["pulsar_det_an_topo_period_ms"]   = prompt_field("Enter the topocentric period in ms - output from TopoBary.py (e.g. 714.50000)", float)
                res["pulsar_det_an_fold_sections"]    = prompt_field("Enter the number of fold sections (e.g. 128)", int)
                res["pulsar_det_an_fft_bins"]         = prompt_field("Enter the number of FFT bins (e.g. 1024)", int)
                res["pulsar_det_an_pulse_width"]      = prompt_field("Enter the ATNF pulse width in ms (e.g. 6.5)", float)
                res["pulsar_det_an_dm"]               = prompt_field("Enter the ATNF dispersion measure DM in pc/cm^3(e.g. 26.7)", float)
                res["pulsar_det_an_ppm_offset"]       = prompt_field("Enter the ppm offset (e.g. 6)", float)
                res["pulsar_det_an_threshold_sigma"]  = prompt_field("Enter the threshold sigma (e.g. 2)", float)
                res["pulsar_det_an_ppm_range_factor"] = prompt_field("Enter the ppm range factor (e.g. 2)", float)
                res["pulsar_det_an_rf_band_mhz"]      = prompt_field("Enter the RF bandwidth in MHz (e.g. 40)", float)
                res["pulsar_det_an_rf_center_mhz"]    = prompt_field("Enter the RF center frequency in MHz (e.g. 35)", float)
                res["pulsar_det_an_roll_avg"]         = prompt_field("Enter the roll average number (e.g. 0)", int)
                res["pulsar_det_an_start_section"]    = prompt_field("Enter the start section (e.g. 0)", int)
                res["pulsar_det_an_end_section"]      = prompt_field("Enter the end section (e.g. 127)", int)
            case "pul_plot":
                pass

    # Warn user about the number of combinations
    keys = list(res.keys())
    value_lists = [res[k] for k in keys]
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
    pprint(combinations)
    print(json.dumps(combinations, indent=4))
    return combinations



