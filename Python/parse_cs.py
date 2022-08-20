#!/usr/bin/python3
import sys
import numpy as np

elem = {
        'cassandra': {},
        'classification': {},
        'cloud9': {},
        'nutch': {},
        'streaming': {}
        }

with open(sys.argv[1]) as f:
    raw = f.read().split('\n')
    head = raw[1].split(';')

    for i in raw[2:-1]:
        name = ''
        for iidx, ii in enumerate(i.split(';')[:-1]):
            if iidx == 0:
                name = ii.split('_')[0]
                continue
            
            if iidx not in elem[name]:
                elem[name][iidx] = []
            elem[name][iidx].append(float(ii))

    print(raw[1])
    for i in elem:
        string = "{};".format(i.capitalize())
        for ii in sorted(elem[i]):
            string = "{}{};".format(string, np.average(elem[i][ii]))
        print(string)
