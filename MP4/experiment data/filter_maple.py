#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Intersecti = .*TT.*"
    filename = sys.argv[4]
    field = sys.argv[6]
    regex = r''
    regex = sys.argv[8]
    index = -1
    with open(filename, 'r') as file:
          csv_reader = csv.reader(file)
          for ind, fie in enumerate(next(csv_reader)):
                    if fie==field:
                              index = ind
          for line in csv_reader:
                    if re.match(regex, line[index]):
                              concat_line = ','.join(line)
                              print(f"{concat_line}\t{1}")
        
