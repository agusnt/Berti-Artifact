#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean
from pprint import pprint

HEIGHT = 1.3

if __name__ == "__main__":
    rc('font', size=13)
    name  = []

    order = {}

    color = {
            'IP-stride': 'snow',
            'No-pref': 'snow',
            'MLOP': 'whitesmoke',
            'IPCP': 'darkgray',
            'Berti': 'black',
            #'IP-stride': 'silver',
            #'Bingo': 'gainsboro',
            #'PPF': 'snow'
            }

    pattern = {
            'IP-stride': '\\\\\\',
            'IPCP': '',
            'No-pref': 'xxxx',
            #'IP-stride': '',
            'MLOP': '',
            'Berti': '',
            #'PPF': '///',
            #'Bingo': '///' 
            }

    translation = {
        'no+no': 'No-pref',
        'ip_stride_24_way_8_ip_phy+no': 'IP-stride',
        'mlop_dpc3+no': 'MLOP',
        'ipcp_isca2020+no': 'IPCP',
        #'ip_stride+no': 'IP-stride',
        #'no+bingo_dpc3': 'Bingo',
        #'no+ppf': 'PPF',
        'vberti+no': 'Berti'
            }

    translation_suite = ['L1D', 'L2', 'LLC']

    with open(sys.argv[1]) as f:
        raw = f.read().split('\n')

        bench = []
        last = None
        jump = False
        for idx, i in enumerate(raw[:-1]):
            splitted = i.split(';')

            if len(splitted) == 1:
                jump = False
                #if splitted[0] != "spec2k17_intensive" and splitted[0] != "gap" and splitted[0] != "GEOMEAN":
                if splitted[0] != "spec2k17_memint" and splitted[0] != "gap":
                    jump = True
                    continue
                bench.append(splitted[0])
            elif not jump:
                pref = "{}+{}".format(splitted[0], splitted[1])

                if splitted[0] == "L1DPref":
                    continue
                if pref not in order:
                    order[pref] = []

                order[pref].append(
                        (
                            float(splitted[-3]),
                            float(splitted[-2]),
                            float(splitted[-1])
                            )
                        )


    fig, ax = plt.subplots(1, 2, figsize=(7,3))

    benchs = {
            #'GAP': 90, 'SPEC17-MemInt': 40, 'GEOMEAN': 30
            #'GAP': 90, 'SPEC17-MemInt': 50
            'SPEC17-MemInt': 50, 'GAP': 90 
            }

    elem = ['(a)', '(b)', '(c)']

    handle = [] 
    for idx, i in enumerate(benchs):
        line = "{}; ".format(i)

        xst = [-.30, -.15, 0, .15, .30]
        WIDTH=.15
        for pos, ii in zip(xst, translation):
            aux = [iii for iii in order[ii][idx]]

            line = "{} {} (L1D: {}, L2C: {}, LLC: {}),".format(line,
                    translation[ii],
                    order[ii][idx][0], order[ii][idx][1], order[ii][idx][2])
            if idx == 0:
                ax[idx].bar(np.arange(len(aux))+pos, aux, width=WIDTH,
                        color=color[translation[ii]], edgecolor='black',
                        hatch=pattern[translation[ii]], label=translation[ii],
                        zorder=3)
            else:
                ax[idx].bar(np.arange(len(aux))+pos, aux, width=WIDTH,
                        color=color[translation[ii]], edgecolor='black',
                        hatch=pattern[translation[ii]], zorder=3)
        print(line)

        top = benchs[i]
        ttop = top + 0
        ax[idx].set_ylim((.95, ttop))
        ax[idx].set_yticks([i for i in np.arange(0, top+.1, 10)])
        ax[idx].set_yticks([i for i in np.arange(0, top+.1, 5)], minor=True)

        ax[idx].set_xticks(np.arange(len(translation_suite)))
        ax[idx].set_xticklabels(translation_suite)

        if idx == 0:
            ax[idx].set_ylabel("Demand MPKI")

        #ax[idx].set_title(i, y=0.0)
        ax[idx].set_xlabel("{} {}".format(elem[idx], i))

        ax[idx].yaxis.grid(True, zorder=1, which='major')
        ax[idx].yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    fig.subplots_adjust(top=0.8, left=0.07, right=0.99, bottom=0.1) 
    legend = fig.legend(loc=9, bbox_to_anchor=(.5, 1.1),
          ncol=5, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))
    
    plt.tight_layout()
    plt.savefig("fig11.pdf", bbox_inches='tight')
