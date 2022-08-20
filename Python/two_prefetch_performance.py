#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean

HEIGHT = 1.4

if __name__ == "__main__":
    rc('font', size=13)
    plt.rcParams['hatch.linewidth'] = 1.5
    name  = []
    order = {
            #'MLOP+MISB': [],
            'MLOP+Bingo': [],
            'MLOP+SPP-PPF': [],
            'IPCP+IPCP': [],
            #'Berti': [],
            #'Berti+MISB': [],
            'Berti+Bingo': [],
            'Berti+SPP-PPF': [],
            'Berti': [],
            }

    geo   = {
            'MLOP+MISB': [],
            'MLOP+Bingo': [],
            'MLOP+SPP-PPF': [],
            'IPCP+IPCP': [],
            #'Berti': [],
            'Berti+MISB': [],
            'Berti+Bingo': [],
            'Berti+SPP-PPF': [],
            'Berti': [],
            }

    color = {
            'MLOP+MISB': 'whitesmoke',
            'MLOP+Bingo': 'whitesmoke',
            'MLOP+SPP-PPF': 'whitesmoke',
            'IPCP+IPCP': 'darkgray',
            #'Berti': 'darkgray',
            'Berti+MISB': 'black',
            'Berti+Bingo': 'black',
            'Berti+SPP-PPF': 'black',
            'Berti': 'black',
            }

    pattern = {
            #'MLOP+MISB': 'xxx',
            'MLOP+Bingo': '///',
            'MLOP+SPP-PPF': '...',
            'IPCP+IPCP': '\\\\\\',
            #'Berti': '',
            #'Berti+MISB': 'xxx',
            'Berti+Bingo': '///',
            'Berti+SPP-PPF': '...',
            'Berti': '',
            }

    translation = {
        #'mlop_dpc3+isb_ideal': 'MLOP+MISB',
        'mlop_dpc3+bingo_dpc3': 'MLOP+Bingo',
        'mlop_dpc3+ppf': 'MLOP+SPP-PPF',
        'ipcp_isca2020+ipcp_isca2020': 'IPCP+IPCP',
        'vberti+no': 'Berti',
        'vberti+bingo_dpc3': 'Berti+Bingo',
        'vberti+ppf': 'Berti+SPP-PPF',
            }

    berti = []

    translation_suite = ['GAP', 'SPEC17-MemInt']
    translation_suite = ['SPEC17-MemInt', 'GAP']

    bench = ['GM']
    bench = []
    
    text = {}

    with open(sys.argv[1]) as f:
        raw = f.read().split('\n')

        last = None
        jump = False
        for idx, i in enumerate(raw[:-1]):
            splitted = i.split(';')

            if len(splitted) == 1:
                jump = False
                if (splitted[0] == "spec2k17_all") or (splitted[0] == "cvp") or (splitted[0] == "cloudsuite"):
                    print(splitted)
                    jump = True
                    continue
                bench.append(splitted[0])
            elif not jump:
                pref = "{}+{}".format(splitted[0], splitted[1])
                if pref not in translation:
                    continue
                height = float(splitted[2])
                #if translation[pref] == 'Berti':
                #    berti.append(height)
                #    continue
                if height > HEIGHT:
                    if translation[pref] not in text:
                        text[translation[pref]] = []
                    if round(height, 1) != HEIGHT:
                        text[translation[pref]].append((len(order[translation[pref]]), height))
                    order[translation[pref]].append(HEIGHT)
                    geo[translation[pref]].append(height)
                else:
                    order[translation[pref]].append(height)
                    geo[translation[pref]].append(height)
                name.append(pref)

    #for i in order:
    #    aux = []
    #    for ii, jj in zip(order[i], berti):
    #        aux.append(ii / jj)
    #    order[i] = aux

    # Idx
    x = np.arange(len(translation_suite))
    fig, ax = plt.subplots(figsize=(7.2,3))
    elem = [-.36, -.24, -.12, 0, .12, .24, .36]
    #elem = [-.35, -.25, -.15, -.05, .05, .15, .25, .35]
    for i, j in zip(order, elem):
        WIDTH=.12
        if i.split('+')[0] == "Berti":
            ax.bar(x+j, order[i], width=WIDTH, edgecolor='snow', zorder=3,
                    color=color[i], hatch=pattern[i], label=i)
            ax.bar(x+j, order[i], width=WIDTH, edgecolor='black', zorder=3,
                    color='none', lw=1.2)
            continue
        ax.bar(x+j, order[i], width=WIDTH, edgecolor='black', zorder=4,
                color=color[i], hatch=pattern[i], label=i)
        if i in text:
            for ii in text[i]:
                ax.text(ii[0]+j-.1, HEIGHT+.01, round(ii[1], 1))

    HEIGHT = 1.15
    min_ = 0.9
    step = .05
    ax.set_ylim((min_, HEIGHT))
    ax.set_yticks([i for i in np.arange(min_, HEIGHT + .01, step)])
    ax.set_yticks([i for i in np.arange(min_, HEIGHT + .01, step/2)], minor=True)

    ax.set_xticks(x)
    ax.set_xticklabels(translation_suite)

    # Horizontal line
    plt.axhline(y=1, color='black', linestyle='-', linewidth=1.5)

    ax.set_ylabel("Speedup")

    #legend = plt.legend(loc=10, bbox_to_anchor=(0.5, .85), prop={'size': 15.5},
    legend = plt.legend(loc=10, bbox_to_anchor=(0.45, 1.3),
          ncol=3, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))

    #plt.axhline(y=1, color='black', linestyle='-', linewidth=1.5, zorder=2)
    
    #lines = [(1.0151183479388, .04, .30), (1.19786829644355, .37, .63), (1.28738942208878, .69, .96)]
    #for i, j, k in lines:
    #    plt.axhline(y=i, xmin=j, xmax=k, color='snow',
    #            linestyle='--', linewidth=1.5, zorder=5, alpha=1)
    #    plt.axhline(y=i, xmin=j, xmax=k, color='black', 
    #            linestyle='-', linewidth=3.5, zorder=4, alpha=1)

    plt.gca().yaxis.grid(True, zorder=1, which='major')
    plt.gca().yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    fig.tight_layout()

    #plt.show()
    #plt.savefig("on_prefetch_performance.png")
    #plt.savefig("on_prefetch_performance.pgf")
    plt.savefig("fig12.pdf", bbox_inches='tight')
