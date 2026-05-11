"""
    A simple scprit for running Peter East's psrsoft alternative
    30.03.2026. Martin Ante Rogošić
"""

import argparse
from lib.funcs import compile_everything
from lib.io import console_program_chain_io
from lib.runners import run_program_chain
program_arg_map = {}
program_chain = [] 

def parse_config_file():
    return None


#This function allows the user to input on the terminal which program chain and which subchain they wish to run
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description="Parser for the stargazer software chain arguments")
    parser.add_argument("-c", "--compile",
                    action="store_true",
                    help="Compile/recompile the C programs")

    #parser.add_argument("--config_file", "-o", help="Path to a config file containing all of the variables neccesary to execute the program chains")
    parser.add_argument("-compile", "-o", help="If enabled the script will compile/recompile the c programs")
    
    parser.parse_args()

    #Compile everything
    #compile_everything()


    #Now we need to ask the user to input all of the data
    program_chain, combinations = console_program_chain_io()

    # Execute the program chain
    run_program_chain(program_chain, combinations)
