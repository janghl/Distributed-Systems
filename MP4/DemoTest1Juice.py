import os
import sys

def juice(filename, key):
    line_count = 0
    full_path = os.path.join(os.getcwd(), f"SDFS/{filename}")
    if os.path.exists(full_path):
        with open(full_path, 'r') as file:
        # reprseents the number of the certain detection_ type 
            line_count = sum(1 for line in file)
            key_val = (key, line_count)

    return key_val

# returns the percent composition of each value of Detection (could possibly be)
if "__main__" == __name__:
    # if len(sys.argv) != 3:
    #     print("Usage: python script.py arg1 arg2")
    #     sys.exit(1)

    # Access command-line arguments
    if len(sys.argv) < 2:
        print("Include filename.")
    filename = sys.argv[1]
    key = sys.argv[2]
    key_val = juice(filename, key)
    print(key_val)


# convert to exe: pyinstaller ./wordcount_maple.py 

