import os
import sys
import csv

# could yield all pairs of (detection, Interconne)
def map(csv_file_path, interconne_type, prefix):
    ret = []
    with open(csv_file_path, 'r') as file:
        # Create a CSV reader object
        csv_reader = csv.DictReader(file)

        # Iterate through each row in the CSV file
        try:
            num = 0
            files = []
            for row in csv_reader:
                # Each row is a list of values from the CSV
                if row['Interconne'] == interconne_type:
                    num+=1
                    #ret.append((row['Detection_'], row['Interconne']))
                    detection = row['Detection_']
                    
                    if detection.strip() == '':
                        detection = ''
                    new_add = f"{detection},{interconne_type}"
                    detection = detection.replace('/','')
                    filename = f"{prefix}_{detection}"
                    temp_file_path = f"temp_{filename}"
                    if os.path.exists(temp_file_path):
                        with open(temp_file_path, 'a') as f:
                            f.write(new_add + '\n')
                    else:
                        with open(temp_file_path, 'w') as f:
                            f.write(new_add + '\n')
                        files.append(filename)
            for filename in files:
                print(filename)
        except Exception as e:
            print(f"An error occurred: {e}")

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
    interconne = sys.argv[3]
    
    map(filename, interconne, prefix)


# convert to exe: pyinstaller ./wordcount_maple.py 

'''
1) IDK
- have each of the 4 possible values {fiber, radio, fiber/radio, None} for Interconne be the keys (4 SDFS Intermediate files)

2) better
- have all the Detection_ values be the keys (k # of SDFS Intermediate files)
- juice will count the number of values per key -> it will result in its percent composition
'''