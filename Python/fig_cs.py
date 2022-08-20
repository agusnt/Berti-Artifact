#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean
from pprint import pprint

if __name__ == "__main__":
    rc('font', size=13)

    color = {
            'IDEAL L1D': 'snow',
            'MLOP': 'whitesmoke',
            'MLOP+Bingo': 'whitesmoke',
            'MLOP+MISB': 'whitesmoke',
            'MLOP+SPP-PPF': 'whitesmoke',
            'IPCP': 'darkgray',
            'IPCP+IPCP': 'darkgray',
            'IPCP+MISB': 'darkgray',
            'Berti': 'black',
            'Berti+Bingo': 'black',
            'Berti+SPP-PPF': 'black',
            'Berti+MISB': 'black',
            #'IP-Stride': 'silver',
            #'Bingo': 'gainsboro',
            #'PPF': 'snow'
            }

    pattern = {
            'IDEAL L1D': '\\\\\\',
            'MLOP': '',
            'MLOP+Bingo': '///',
            'MLOP+MISB': 'xxx',
            'MLOP+SPP-PPF': '...',
            'IPCP': '',
            'IPCP+IPCP': '\\\\\\',
            'IPCP+MISB': 'xxx',
            'Berti': '',
            'Berti+Bingo': '///',
            'Berti+SPP-PPF': '...',
            'Berti+MISB': 'xxx',
            }

    translation = {
        'PRACTICAL_PERFECT_L1D_': 'IDEAL L1D',
        'mlop_dpc3_no': 'MLOP',
        'mlop_dpc3_bingo_dpc3': 'MLOP+Bingo',
        'mlop_dpc3_ppf': 'MLOP+SPP-PPF',
        #'mlop_dpc3_isb_ideal': 'MLOP+MISB',
        'ipcp_isca2020_no': 'IPCP',
        'ipcp_isca2020_ipcp_isca2020': 'IPCP+IPCP',
        #'ipcp_isca2020_isb_ideal': 'IPCP+MISB',
        'vberti_no': 'Berti',
        'vberti_ppf': 'Berti+SPP-PPF',
        'vberti_bingo_dpc3': 'Berti+Bingo',
        #'vberti_isb_ideal': 'Berti+MISB'
            }

    dic = {}

    with open(sys.argv[1]) as f:
        raw = f.read().split('\n')

        element = raw[0].split(';')[1:]

        for i in raw[1:-1]:
            aux = i.split(';')
            dic[aux[0]] = []
            for ii in aux[1:-1]:
                dic[aux[0]].append(float(ii))

    fig, ax = plt.subplots(figsize=(7,1.5))

    to_graph = [
            #'PRACTICAL_PERFECT_L1D_',
            'mlop_dpc3_no', 'mlop_dpc3_bingo_dpc3', 'mlop_dpc3_ppf',
            'ipcp_isca2020_no', 'ipcp_isca2020_ipcp_isca2020', 
            'vberti_no', 
            'vberti_bingo_dpc3',
            'vberti_ppf'
            ]

    y = {}
    for i in dic:
        for ii, jj in zip(dic[i], element):
            if jj in to_graph:
                if jj not in y:
                    y[jj] = []
                y[jj].append(ii)

    #xst = [-.36, -.28, -.20, -.12, -.04, .04, .12, .20, .28, .36]
    xst = [-.32, -.24, -.16, -.08, .0, .08, .16, .24, .32]
    #xst = [-.35, -.25, -.15, -.05, .05, .15, .25, .35]
    WIDTH=.1
    WIDTH=.08
    line = ""
    for pos, i in zip(xst, to_graph):
        line = "{} {} (Cassandra: {}, Classification: {}, Cloud9: {}, Nutch: {}, Streaming: {});".format(line,
                translation[i], y[i][0], y[i][1], y[i][2], y[i][3], y[i][4])
        if i.split('_')[0] == "vberti":
            ax.bar(np.arange(len(y[i])) + pos, y[i], width=WIDTH,
                    color=color[translation[i]], edgecolor='snow',
                    hatch=pattern[translation[i]], zorder=3,
                    label=translation[i])
            ax.bar(np.arange(len(y[i])) + pos, y[i], width=WIDTH,
                    color='none', edgecolor='black', zorder=4, lw=1.2)
        else:
            ax.bar(np.arange(len(y[i])) + pos, y[i], width=WIDTH,
                    color=color[translation[i]], edgecolor='black',
                    hatch=pattern[translation[i]], zorder=4,
                    label=translation[i])
    print(line)

    ax.set_xticks(np.arange(len(dic)))
    ax.set_xticklabels([i for i in dic], rotation=15, ha='center')

    #top = 1.15
    top = 1.10
    ttop = top
    min_ = .95
    ax.set_ylim(min_, ttop)
    ax.set_yticks([i for i in np.arange(min_, top+.001, 0.05)])
    ax.set_yticks([i for i in np.arange(min_, top+.001, 0.025)], minor=True)

    ax.yaxis.grid(True, zorder=1, which='major')
    ax.yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    ax.axhline(y=1, color='black', linewidth=1.5)

    ax.set_ylabel("Speedup")

    #legend = plt.legend(loc=10, bbox_to_anchor=(.45, 1.6),
    legend = plt.legend(loc=10, bbox_to_anchor=(.428, 1.5),
          ncol=3, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))

    #plt.tight_layout()
    plt.savefig("fig18.pdf", bbox_inches='tight')
    #plt.show()
    sys.exit()
