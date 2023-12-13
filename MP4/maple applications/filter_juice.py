#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Intersecti = *TT*"
    for line in sys.stdin:
              result = line.strip().split('\t', 1)[0]
              print(f"{result}\t{' '}")
        

def juice(filename,key,sqlstr):
    full_path = os.path.join(os.getcwd(), f"SDFS/{filename}")
    if os.path.exists(full_path):
        with open(full_path, 'r') as file:
            for line in file:
              result = line.strip().split('\t', 1)[0]
              print(f"{result}\t{' '}")


if "__main__" == __name__:
    # if len(sys.argv) != 3:
    #     print("Usage: python script.py arg1 arg2")
    #     sys.exit(1)

    # Access command-line arguments
    if len(sys.argv) < 2:
        print("Include filename.")
    intermediate_filename = sys.argv[1]
    key = sys.argv[2]
    sqlstr = sys.argv[3]
    juice(intermediate_filename,key,sqlstr)
    # words = []
    '''
    for pair in contents:
        try:
            # Open the file in write mode ('w')
            filename = f"{prefix}_{pair[0]}"
            if os.path.exists(filename):
                with open(filename, 'a') as file:
                    # Write content to the file
                    file.write(f"{pair[0]},{1}")
            else:
                with open(filename, 'w') as file:
                    # Write content to the file
                    file.write(f"{pair[0]},{1}")
                    words.append(filename)
        except Exception as e:
            print(f"An error occurred: {e}")
    '''

# convert to exe: pyinstaller ./wordcount_maple.py 