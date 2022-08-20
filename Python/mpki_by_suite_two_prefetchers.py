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
    plt.rcParams['hatch.linewidth'] = 1.5

    order = {}

    color = {
            #'NOPF': 'snow',
            #'MLOP+MISB': 'whitesmoke',
            'MLOP': 'whitesmoke',
            'MLOP+Bingo': 'whitesmoke',
            'MLOP+SPP-PPF': 'whitesmoke',
            'IPCP': 'darkgray',
            'IPCP+IPCP': 'darkgray',
            'Berti': 'black',
            #'Berti+MISB': 'darkgray',
            'Berti+Bingo': 'black',
            'Berti+SPP-PPF': 'black',
            }

    pattern = {
            'NOPF': '\\\\\\',
            'MLOP': '',
            'MLOP+MISB': 'xxx',
            'MLOP+Bingo': '///',
            'MLOP+SPP-PPF': '...',
            'IPCP': '',
            'IPCP+IPCP': '\\\\\\',
            'Berti': '',
            'Berti+MISB': 'xxx',
            'Berti+Bingo': '///',
            'Berti+SPP-PPF': '...',
            }

    translation = {
        #'no+no': 'NOPF',
        #'mlop_dpc3+isb_ideal': 'MLOP+MISB',
        'mlop_dpc3+no': 'MLOP',
        'mlop_dpc3+bingo_dpc3': 'MLOP+Bingo',
        'mlop_dpc3+ppf': 'MLOP+SPP-PPF',
        'ipcp_isca2020+no': 'IPCP',
        'ipcp_isca2020+ipcp_isca2020': 'IPCP+IPCP',
        'vberti+no': 'Berti',
        #'vberti+isb_ideal': 'Berti+MISB',
        'vberti+bingo_dpc3': 'Berti+Bingo',
        'vberti+ppf': 'Berti+SPP-PPF'
            }

    translation_suite = ['L1D', 'L2', 'LLC']
    translation_suite = ['L2', 'LLC']

    with open(sys.argv[1]) as f:
        raw = f.read().split('\n')

        bench = []
        last = None
        jump = False
        for idx, i in enumerate(raw[:-1]):
            splitted = i.split(';')

            if len(splitted) == 1:
                jump = False
                if splitted[0] != "spec2k17_intensive" and splitted[0] != "gap":
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

    geo = {}
    for i in order:
        if i not in geo:
            geo[i] = []

        a = []
        b = []
        c = []
        for ii in order[i]:
            a.append(ii[0])
            b.append(ii[1])
            c.append(ii[2])
        geo[i].append(gmean(a))
        geo[i].append(gmean(b))
        geo[i].append(gmean(c))

    for i in geo:
        aux = (geo[i][0], geo[i][1], geo[i][2])
        order[i].append(aux) 

    #fig, ax = plt.subplots(1, 3, figsize=(7,3))
    fig, ax = plt.subplots(1, 2, figsize=(7,2.5))

    benchs = {
            #'GAP': 50, 'SPEC17-MemInt': 15
            'SPEC17-MemInt': 15, 'GAP': 50
            }
    bottom = {
            #'GAP': 15, 'SPEC17-MemInt': 5
            'SPEC17-MemInt': 5, 'GAP': 15
            }
    elem = ['(a)', '(b)', '(c)']

    handle = [] 
    for idx, i in enumerate(benchs):
        xst = [-.35, -.21, -.07, .07, .21, .35]
        xst = [-.35, -.25, -.15, -.05, .05, .15, .25, .35]

        line = "{}; ".format(i)
        for pos, ii in zip(xst, translation):
            aux = [iii for iii in order[ii][idx]][1:]
            WIDTH = .14
            WIDTH = .1

            line = "{} {} (L2: {}, LLC: {})".format(line, translation[ii],
                    order[ii][idx][1], order[ii][idx][2])

            if translation[ii].split('+')[0] == 'Berti':
                ax[idx].bar(np.arange(len(aux))+pos, aux, width=WIDTH,
                        color=color[translation[ii]], edgecolor='snow',
                        hatch=pattern[translation[ii]], label=translation[ii],
                        zorder=2)
                ax[idx].bar(np.arange(len(aux))+pos, aux, width=WIDTH,
                        color='none', edgecolor='black',
                        zorder=2, lw=1.2)
                continue
            ax[idx].bar(np.arange(len(aux))+pos, aux, width=WIDTH,
                    color=color[translation[ii]], edgecolor='black',
                    hatch=pattern[translation[ii]], label=translation[ii],
                    zorder=3)
        print(line)

        top = benchs[i]
        ttop = top + 0
        ax[idx].set_ylim((bottom[i], ttop))
        ax[idx].set_yticks([i for i in np.arange(bottom[i], top+.1, 5)])
        ax[idx].set_yticks([i for i in np.arange(bottom[i], top+.1, 2.5)], minor=True)

        ax[idx].set_xticks(np.arange(2))
        ax[idx].set_xticklabels(translation_suite)

        if idx == 0:
            ax[idx].set_ylabel("Demand MPKI")

        #ax[idx].set_title(i, y=0.0)
        ax[idx].set_xlabel("{} {}".format(elem[idx], i))

        ax[idx].yaxis.grid(True, zorder=1, which='major')
        ax[idx].yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    fig.subplots_adjust(top=0.8, left=0.07, right=0.99, bottom=0.1) 
    handles,labels = ax[1].get_legend_handles_labels()
    legend = fig.legend(handles, labels, loc=9, bbox_to_anchor=(.52, 1.32),
          ncol=3, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))
    
    plt.tight_layout()
    plt.savefig("fig13.pdf", bbox_inches='tight')
