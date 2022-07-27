#!/usr/bin/python3
import re, os

def iterateOverDir(fdir, grep):
    dic = {}
    for fname in os.listdir(fdir):
        #name = fname.split('---')[0] 
        name = fname 
        if name not in dic:
            dic[name] = []
        dic[name].append(readFile("{}/{}".format(fdir, fname), grep))
    return dic

def readFile(fname, grep):
    with open(fname) as f:
        raw = f.read()
        line = re.findall(grep, raw)
    return line

