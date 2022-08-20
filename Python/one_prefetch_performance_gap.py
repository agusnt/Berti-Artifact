#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean

HEIGHT = 1.15

if __name__ == "__main__":
    rc('font', size=13)
    name  = []
    order = {
            'MLOP': [],
            'IPCP': [],
            #'IP-Stride': [],
            #'BINGO': [],
            #'PPF': [],
            'Berti': [],
            }

    geo   = {
            'MLOP': [],
            'IPCP': [],
            #'IP-Stride': [],
            #'BINGO': [],
            #'PPF': [],
            'Berti': [],
            }

    color = {
            'IDEAL': 'whitesmoke',
            'MLOP': 'whitesmoke',
            'IPCP': 'darkgray',
            'Berti': 'black',
            #'IP-Stride': 'silver',
            #'BINGO': 'gainsboro',
            #'PPF': 'snow'
            }

    pattern = {
            'IDEAL': '',
            'IPCP': '',
            #'IP-Stride': '',
            'MLOP': '',
            'Berti': '',
            #'PPF': '///',
            #'BINGO': '///' 
            }

    translation = {
        'mlop_dpc3+no': 'MLOP',
        'ipcp_isca2020+no': 'IPCP',
        #'ip_stride+no': 'IP-Stride',
        #'no+bingo_dpc3': 'BINGO',
        #'no+ppf': 'PPF',
        'vberti+no': 'Berti'
            }

    translation_suite = ['GAP']

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
                print(splitted[0])
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
                if height > HEIGHT:
                    if translation[pref] not in text:
                        text[translation[pref]] = []
                    text[translation[pref]].append((len(order[translation[pref]]), height))
                    order[translation[pref]].append(HEIGHT)
                    geo[translation[pref]].append(height)
                else:
                    order[translation[pref]].append(height)
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
    x = np.arange(len(bench))
    fig, ax = plt.subplots(figsize=(7,2.5))
    elem = [-.25, 0, .25]
    for i, j in zip(order, elem):
        ax.bar(x+j, order[i], width=.25, edgecolor='black', zorder=3,
                color=color[i], hatch=pattern[i], label=i)
        if i in text:
            for ii in text[i]:
                ax.text(ii[0]+j-.1, HEIGHT+.01, round(ii[1], 1))

    #for idx, i in enumerate(translation_suite):
    #    line = i
    #    for ii in order:
    #        line = "{}, {}: {};".format(line, ii, order[ii][idx])
    #    print(line)

    below = .9
    step = .05
    ax.set_ylim((below, HEIGHT))
    ax.set_yticks([i for i in np.arange(below, HEIGHT + .01, step)])
    ax.set_yticks([i for i in np.arange(below, HEIGHT + .01, step/2)], minor=True)

    x = [0]
    ax.set_xticks(x)
    ax.set_xticklabels(translation_suite)

    ax.set_ylabel("Speedup")

    #legend = plt.legend(loc=10, bbox_to_anchor=(0.5, .85), prop={'size': 15.5},
    legend = plt.legend(loc=10, bbox_to_anchor=(0.5, 1.15),
          ncol=3, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))

    plt.gca().yaxis.grid(True, zorder=1, which='major')
    plt.gca().yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    # Horizontal line
    plt.axhline(y=1, color='black', linestyle='-', linewidth=1.5)

    fig.tight_layout()

    plt.savefig("fig8.pdf", bbox_inches='tight')
