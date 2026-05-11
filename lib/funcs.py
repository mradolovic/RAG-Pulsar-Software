import subprocess

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
    subprocess.run(["make"], capture_output=False, text=True, cwd="./RTL/")

 
