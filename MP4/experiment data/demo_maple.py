#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Interconne = fiber"
    filename = sys.argv[4]
    field = sys.argv[6]
    interconne = r''
    interconne = sys.argv[8]
    index = -1
    detectionIndex = 9
    dic = {}
    total = 0
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
              print(f"key:{val/total*100}%\t{1}")
        
