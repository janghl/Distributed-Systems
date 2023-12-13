#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from D1.csv and D2.csv where D1.ID=D2.name"
    file1 = sys.argv[4]
    file2 = sys.argv[6]
    query = sys.argv[8]
    field1 = query.split('=')[0].split('.')[1]
    field2 = query.split('=')[1].split('.')[1]
    index1 = -1
    index2 = -1
    with open(file1, 'r') as f1:
          with open(file2, 'r') as f2:
                    csv_reader1 = csv.reader(f1)
                    csv_reader2 = csv.reader(f2)
                    for ind, fie in enumerate(next(csv_reader1)):
                              if ind==0:
                                        fie = fie[1:]
                              if fie.strip().lower()==field1.strip().lower():
                                        index1 = ind
                    for ind, fie in enumerate(next(csv_reader2)):
                              if ind==0:
                                        fie = fie[1:]
                              if fie==field2:
                                        index2 = ind
                    for line1 in csv_reader1:
                              f2.seek(0)
                              for line2 in csv_reader2:
                                        if len(line1)>index1 and len(line2)>index2 and line1[index1]==line2[index2]:
                                                  concat_line = ','.join(list(set(line1 + line2)))
                                                  print(f"{concat_line}\t{1}")
          
