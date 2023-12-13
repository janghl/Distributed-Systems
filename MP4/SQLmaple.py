import os
import sys
import csv

# ex) SELECT ALL FROM DATASET WHERE Interconne="fiber"

def map(filename, field, field_value):
    ret = []
    with open(csv_file_path, 'r') as file:
        csv_reader = csv.reader(file)
        # keys are the Interconne values (passed in field in query), value are the entire rows
        try:
            for row in csv_reader:
                if row['field'] == field_value:
                    new_add = f"{row['field']},{str(row)}"
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
    condition = sys.argv[3]
    
    map(filename, condition)


# convert to exe: pyinstaller ./wordcount_maple.py 

'''
1) IDK
- have each of the 4 possible values {fiber, radio, fiber/radio, None} for Interconne be the keys (4 SDFS Intermediate files)

2) better
- have all the Detection_ values be the keys (k # of SDFS Intermediate files)
- juice will count the number of values per key -> it will result in its percent composition
'''