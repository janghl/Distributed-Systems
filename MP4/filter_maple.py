#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re

def contains_regex_pattern(input_string, regex_pattern):
    match = re.search(regex_pattern, input_string)
    return bool(match)

if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Intersecti = .*TT.*"
    prefix = sys.argv[1]
    sqlstr = sys.argv[2]
    sqlstr = sqlstr.strip().split('^')
    
    filename = sqlstr[3]
    # field = sqlstr[5]
    rgx = sqlstr[5]
    
    regex = re.compile(rgx)
    # regex = r"Video,Radio"
    index = -1
    words = []
    with open(filename, 'r') as file:
          csv_reader = csv.reader(file)
        #   for ind, fie in enumerate(next(csv_reader)):
        #             if fie==field:
        #                       index = ind
          for line in csv_reader:
                    row_string = ','.join(line)
                    if contains_regex_pattern(row_string, regex):
                            # print("hi")
                            rgx = rgx.replace(',','')
                            filename = f"{prefix}_{rgx}"
                            temp_filename = f"temp_{filename}"
                            if os.path.exists(temp_filename):
                                with open(temp_filename, 'a') as file:
                                    # Write content to the file
                                    file.write(f"{row_string}\t{1}\n")
                            else:
                                with open(temp_filename, 'w') as file:
                                    # Write content to the file
                                    file.write(f"{row_string}\t{1}\n")
                                    words.append(filename)    
                                    
    for filename in words:
        print(filename)
                                                  
        
