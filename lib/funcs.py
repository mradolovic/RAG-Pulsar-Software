import subprocess
import os
import sys

def confirm(prompt="Continue?", default=True):
    hint = "[Y/n]" if default else "[y/N]"
    while True:
        answer = input(f"{prompt} {hint}: ").strip().lower()
        if answer == "":
            return default
        if answer in ("y", "yes"):
            return True
        if answer in ("n", "no"):
            return False
        print("Please enter y or n.")


def compile_everything():
    #TODO: Here is only the command to compile the program chain 1, when we have the code and the
    #need we will have to add the compilation of the second program chain
    rtl_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "RTL")
    ext = ".exe" if sys.platform == "win32" else ".out"
    bin_dir = os.path.join(rtl_dir, "bin")

    binaries = ["FileTrim", "RAFFT22Lg", "RTLChannel4bin", "pulsar_det_an_v4", "rapulsar2con"]

    # If any binary is missing, clean and do a full rebuild
    if any(not os.path.exists(os.path.join(bin_dir, f"{b}{ext}")) for b in binaries):
        subprocess.run(["make", "clean"], capture_output=False, text=True, cwd=rtl_dir)

    subprocess.run(["make", "all"], capture_output=False, text=True, cwd=rtl_dir)

 
