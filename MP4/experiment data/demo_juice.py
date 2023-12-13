#!/usr/bin/env python3  
# -*-coding:utf-8 -*
import os
import sys
import csv
import re


if "__main__" == __name__:
    # ex) -mapper "/path/to/map.py select all from Test.csv where Intersecti = *TT*"
    for line in sys.stdin:
              result = line.strip().split('\t', 1)[0]
              print(f"{result}\t{' '}")
        
