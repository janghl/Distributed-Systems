#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Intersecti = .*TT.*"
    prefix = sys.argv[2]
    sqlstr = sys.argv[3]
    sqlstr = sqlstr.strip().split('^')
    filename = sqlstr[3]
    field = sqlstr[5]
    regex = r''
    regex = sqlstr[7]
    index = -1
    words = []
    with open(filename, 'r') as file:
          csv_reader = csv.reader(file)
          for ind, fie in enumerate(next(csv_reader)):
                    if fie==field:
                              index = ind
          for line in csv_reader:
              for i in range(20):
                    if re.match(regex, line[i]):
                            concat_line = ':'.join(line)
                            filename = f"{prefix}_{concat_line}"
                            filename=filename.replace(' ','').replace('/','')[0:250]
                            temp_filename = 'temp_'+f"{filename}"
                            if os.path.exists(temp_filename):
                                with open(temp_filename, 'a') as file:
                                    # Write content to the file
                                    file.write(f"{concat_line}\t{1}\n")
                            else:
                                with open(temp_filename, 'w') as file:
                                    # Write content to the file
                                    file.write(f"{concat_line}\t{1}\n")
                                    words.append(filename)
                    continue
                               

