import subprocess

def run_program_chain(program_chain, combinations):
    for program in program_chain:
        match program:
            case "rtl_srd":
                pass
            case "retchannel4bin":
                for combination in combinations:
                    subprocess.run()
                
    
