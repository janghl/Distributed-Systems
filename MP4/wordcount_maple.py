import os
import sys

def map(filename):
    ret = []
    with open(filename, 'r') as file:
        for line in file:
            line = line.split(" ")
            for w in line:
                if w[-1] == '\n':
                    ret.append((w[:-1],1))
                else:
                    ret.append((w, 1))

    return ret

if "__main__" == __name__:
    # if len(sys.argv) != 3:
    #     print("Usage: python script.py arg1 arg2")
    #     sys.exit(1)

    # Access command-line arguments
    # if len(sys.argv) < 2:
    #     print("Include filename.")
    filename = sys.argv[1]
    prefix = sys.argv[2]
    contents = map(filename)
    words = []
    
    for pair in contents:
        try:
            # Open the file in write mode ('w')
            filename = f"{prefix}_{pair[0]}"
            temp_filename = f"temp_{filename}"
            if os.path.exists(temp_filename):
                with open(temp_filename, 'a') as file:
                    # Write content to the file
                    file.write(f"{pair[0]},{1}\n")
            else:
                with open(temp_filename, 'w') as file:
                    # Write content to the file
                    file.write(f"{pair[0]},{1}\n")
                    words.append(filename)
        except Exception as e:
            pass
            #print(f"An error occurred: {e}")
    for filename in words:
        print(filename)

# convert to exe: pyinstaller ./wordcount_maple.py 