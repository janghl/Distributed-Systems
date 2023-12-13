import os
import sys
import csv

# ex) SELECT ALL FROM DATASET WHERE Interconne="fiber"

def juice(filename):
    ret = []
    with open(filename, 'r') as file:
        # reprseents the number of the certain detection_ type 
        line_count = sum(1 for line in file)

    return ret

if "__main__" == __name__:
    # if len(sys.argv) != 3:
    #     print("Usage: python script.py arg1 arg2")
    #     sys.exit(1)

    # Access command-line arguments
    if len(sys.argv) < 2:
        print("Include filename.")
    filename = sys.argv[1]
    prefix = sys.argv[2]
    condition = sys.argv[3]
    
    juice(filename)


# convert to exe: pyinstaller ./wordcount_maple.py 
