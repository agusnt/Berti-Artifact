#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean
from pprint import pprint

if __name__ == "__main__":
    rc('font', size=13)
    name  = []
    order = [
                {
                    'MLOP': [],
                    'IPCP': [],
                    'Berti': [],
                },
                {
                    'MLOP+Bingo': [],
                    'MLOP+SPP-PPF': [],
                    'IPCP+IPCP': [],
                    'Berti+Bingo': [],
                    'Berti+SPP-PPF': [],
                }
            ]

    geo   = {
            'MLOP': [],
            'MLOP+Bingo': [],
            'MLOP+SPP-PPF': [],
            'IPCP': [],
            'IPCP+IPCP': [],
            'Berti': [],
            'Berti+Bingo': [],
            'Berti+SPP-PPF': [],
            }


    color = {
            'MLOP': 'whitesmoke',
            'MLOP+Bingo': 'whitesmoke',
            'MLOP+SPP-PPF': 'whitesmoke',
            'IPCP': 'darkgray',
            'IPCP+IPCP': 'darkgray',
            'Berti': 'black',
            'Berti+SPP-PPF': 'black',
            'Berti+Bingo': 'black',
            }

    pattern = {
            'MLOP': '',
            'MLOP+Bingo': '///',
            'MLOP+SPP-PPF': '...',
            'IPCP': '',
            'IPCP+IPCP': '\\\\\\',
            'Berti': '',
            'Berti+Bingo': '///',
            'Berti+SPP-PPF': '...',
            }

    translation = {
        'mlop_dpc3+no': 'MLOP',
        'ipcp_isca2020+no': 'IPCP',
        'mlop_dpc3+bingo_dpc3': 'MLOP+Bingo',
        'mlop_dpc3+ppf': 'MLOP+SPP-PPF',
        'ipcp_isca2020+ipcp_isca2020': 'IPCP+IPCP',
        'vberti+no': 'Berti',
        'vberti+ppf': 'Berti+SPP-PPF',
        'vberti+bingo_dpc3': 'Berti+Bingo'
            }

    translation_suite = ['2-Core', '4-Core']
    translation_suite = ['4-Core']

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
                #if (splitted[0] == "spec2k17_all"):
                #    print(splitted)
                #    jump = True
                #    continue
                bench.append(splitted[0])
            elif not jump:
                pref = "{}+{}".format(splitted[0], splitted[1])
                if pref not in translation:
                    continue
                height = float(splitted[2])
                dx = 0
                if translation[pref] in order[1]:
                    dx = 1
                order[dx][translation[pref]].append(height)
                geo[translation[pref]].append(height)
                name.append(pref)

    #for i in order:
    #    aux = gmean(geo[i])
    #    if aux > HEIGHT:
    #        text[i].append((len(order[i]), aux))
    #        order[i].append(HEIGHT)
    #    else:
    #        order[i].append(aux)

    # Idx
    x = np.arange(1)
    fig, ax = plt.subplots(figsize=(7,2.))
    xst = [-.35, -.21, -.07, .07, .21, .35]
    xst = [-.35, -.25, -.15, -.05, .05, .15, .25, .35]
    xst = [
            [-.15, 0, .15],
            [-.3, -.15, 0, .15, .3],
            ]
    for ii in range(0, 2):
        for i, j in zip(order[ii], xst[ii]):
            WIDTH = .14
            WIDTH = .15
            if i.split('+')[0] == "Berti":
                ax.bar(ii+x+j, order[ii][i], width=WIDTH, edgecolor='snow', zorder=3,
                        color=color[i], hatch=pattern[i], label=i)
                ax.bar(ii+x+j, order[ii][i], width=WIDTH, edgecolor='black', zorder=3,
                        color='none')
            else:
                ax.bar(ii+x+j, order[ii][i], width=WIDTH, edgecolor='black', zorder=4,
                        color=color[i], hatch=pattern[i], label=i)

    #line = ""
    #for i in order:
    #    line = "{} {}: {};".format(line, i, order[i][0])
    #print(line)

    top = 1.18
    bottom = 1.08
    step = 0.02
    ttop = top
    ax.set_ylim((bottom, ttop))
    ax.set_yticks([i for i in np.arange(bottom, top+.001, step)])
    ax.set_yticks([i for i in np.arange(bottom, top+.001, step/2)], minor=True)

    ax.set_xticks([0, 1])
    ax.set_xticklabels(["Single-level", "Multi-level"])

    ax.set_ylabel("Speedup")

    #legend = plt.legend(loc=10, bbox_to_anchor=(0.5, .85), prop={'size': 15.5},
    legend = plt.legend(loc=10, bbox_to_anchor=(0.45, 1.4),
          ncol=3, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))

    plt.gca().yaxis.grid(True, zorder=1, which='major')
    plt.gca().yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    # Horizontal line
    plt.axhline(y=1, color='black', linestyle='-', linewidth=1.5)

    #fig.tight_layout()

    #plt.show()
    #plt.savefig("on_prefetch_performance.png")
    #plt.savefig("on_prefetch_performance.pgf")
    plt.savefig("multi_performance-rebuttal.pdf", bbox_inches='tight')
