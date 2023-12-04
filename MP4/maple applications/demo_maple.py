#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Interconne = fiber"
    prefix = sys.argv[2]
    sqlstr = sys.argv[3]
    sqlstr = sqlstr.strip().split('^')
    filename = sqlstr[3]
    field = sqlstr[5]
    interconne = r''
    interconne = sqlstr[7]
    index = -1
    detectionIndex = 9
    dic = {}
    total = 0
    words = []
    with open(filename, 'r') as file:
          csv_reader = csv.reader(file)
          for ind, fie in enumerate(next(csv_reader)):
                    if fie==field:
                              index = ind
          for line in csv_reader:
                    if re.match(interconne.lower(), line[index].lower()):
                        target = line[detectionIndex]
                        if target != ' ':
                            if target in dic:
                                dic[target] += 1
                                total += 1
                            else:
                                dic[target] = 1
                                total += 1
          for key, val in dic.items():
            concat_line = (f"key:{val/total*100}%\t{1}")
            filename = f"{prefix}_{concat_line}"
            temp_filename = f"temp_{filename}"
            if os.path.exists(temp_filename):
                with open(temp_filename, 'a') as file:
                    # Write content to the file
                    file.write(f"{concat_line}\t{1}\n")
            else:
                with open(temp_filename, 'w') as file:
                    # Write content to the file
                    file.write(f"{concat_line}\t{1}\n")
                    words.append(filename)           
    for filename in words:
        print(filename)
        
