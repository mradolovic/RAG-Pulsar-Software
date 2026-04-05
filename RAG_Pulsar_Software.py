"""
    A simple scprit for running Peter East's psrsoft alternative
    30.03.2026. Martin Ante Rogošić
"""

import os
import argparse
from utils.utils import confirm
from utils.io import console_program_chain_io
program_arg_map = {}
program_chain = [] 

def parse_config_file():
    return None


#This function allows the user to input on the terminal which program chain and which subchain they wish to run
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description="Parser for the stargazer software chain arguments")
    parser.add_argument("--config_file", "-o", help="Path to a config file containing all of the variables neccesary to execute the program chains")

