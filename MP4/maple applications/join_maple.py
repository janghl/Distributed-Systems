#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select^all^from^D1.csv^and^D2.csv^where^D1.ID=D2.name"
    prefix = sys.argv[2]
    sqlstr = sys.argv[3]
    sqlstr = sqlstr.strip().split('^')
    file1 = sqlstr[3]
    file2 = sqlstr[5]
    query = sqlstr[7]
    field1 = query.split('=')[0].split('.')[1]
    field2 = query.split('=')[1].split('.')[1]
    index1 = -1
    index2 = -1
    words = []
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
                                        concat_line = ':'.join(list(set(line1 + line2)))
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
    for filename in words:
        print(filename)
                                                  
                                                  
          