#!/usr/bin/python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
from scipy.stats import gmean

HEIGHT = 1.5

if __name__ == "__main__":
    rc('font', size=13)
    ylimit_trans = {
            'gap': 1.1,
            'cpu': 1.4 
            }
    y_limit = ylimit_trans[sys.argv[2]]

    name  = []
    order = {
            'MLOP': [],
            'IPCP': [],
            'Berti': [],
            }

    geo   = {
            'MLOP': [],
            'IPCP': [],
            'Berti': [],
            }

    color = {
            'MLOP': 'whitesmoke',
            'IPCP': 'darkgray',
            'Berti': 'black',
            }

    pattern = {
            'MLOP': '',
            'IPCP': '',
            'Berti': '',
            }

    translation = {
        'mlop_dpc3_no': 'MLOP',
        'ipcp_isca2020_no': 'IPCP',
        #'vberti_ross_hash_launch_limit_4_only_l1_80_l2_35_no': 'Berti',
        #'vberti_ross_hash_no': 'Berti',
        #'vberti_no': 'Berti',
        #'vberti_ross_hash_launch_lanzar_8_filtro_no': 'Berti',
        'vberti_no': 'Berti',
        #'vberti_ross_hash_last_filter_no': 'Berti',
        #'vberti_last_filter_no': 'Berti',
            }

    bars = [] 
    dxbar = {}

    bench = []
    text = {}

    with open(sys.argv[1]) as f:
        raw = f.read().split('\n')

        last = None
        for idx, i in enumerate(raw[:-1]):
            splitted = i.split(';')

            if len(splitted) == 1:
                bench.append(splitted[0])
            else:
                if splitted[0] == "":
                    for iidx, ii in enumerate(splitted):
                        if ii in translation:
                            dxbar[iidx] = translation[ii]
                elif len(splitted[0].split('.')) == 1:
                    continue
                else:
                    bar_name = splitted[0].split('.')[0]
                    if bar_name[0] == '6':
                        bar_name = splitted[0].split('.')[1][:-1]
                    bars.append(bar_name)

                    for iidx, ii in enumerate(splitted):
                        if iidx in dxbar:
                            height = float(splitted[iidx])
                            if height > y_limit:
                                if dxbar[iidx] not in text:
                                    text[dxbar[iidx]] = []
                                text[dxbar[iidx]].append((len(order[dxbar[iidx]]), height))
                                order[dxbar[iidx]].append(y_limit)
                                geo[dxbar[iidx]].append(height)
                            else:
                                order[dxbar[iidx]].append(height)
                                geo[dxbar[iidx]].append(height)
    for i in geo:
        aux = gmean(geo[i])
        if aux > y_limit:
            text[i].append((len(geo[i]), aux))
            order[i].append(y_limit)
        else:
            order[i].append(aux)

    if sys.argv[2] != "gap":
        bars.append("GEOMEAN-MemInt")
    else:
        bars.append("GEOMEAN")

    arrows_format = {
            'MLOP': (-.90, .1),
            'IPCP': (-0.25, .3),
            'Berti': (0.35, 0.1),
            }

    with_arrows = {
            'cpu': [1, 5, 8, 9, 11, 35, 37, 40]
            }
    # Idx
    x = np.arange(len(bars))
    if sys.argv[2] == "gap":
        fig, ax = plt.subplots(figsize=(15,4))
    else:
        fig, ax = plt.subplots(figsize=(15,5))
    elem = [-.25, 0, .25]
    for i, j in zip(order, elem):
        line = "{};".format(i)
        for ii, jj in zip(order[i], bars):
            line = "{} {}: {} | ".format(line, jj, ii)
        print(line)
        ax.bar(x+j, order[i], width=.25, edgecolor='black', zorder=3,
                color=color[i], hatch=pattern[i], label=i)

        if i in text:
            for ii in text[i]:
                if j == -0.25:
                    tx = ii[0] + j + .10
                    ty = y_limit + .07
                elif j == 0:
                    tx = ii[0] + j - .2
                    ty = y_limit
                else:
                    tx = ii[0] + j - .40
                    ty = y_limit

                if sys.argv[2] in with_arrows and ii[0] in with_arrows[sys.argv[2]]:
                    arrowx = arrows_format[i][0]
                    arrowy = arrows_format[i][1]

                    plt.annotate(round(ii[1], 1), (ii[0]+j,y_limit-.01), xytext = (tx+arrowx, ty+arrowy), 
                        #arrowprops=dict(facecolor='black', arrowstyle='->'), 
                        arrowprops=dict(facecolor='black', arrowstyle='->'), 
                        horizontalalignment='left')
                else:
                    ax.text(tx, ty, round(ii[1], 1))


    ax.set_xlim((-0.5, len(x)-.5))
    ax.set_xticks(x)
    ax.set_xticklabels(bars, rotation=45, ha='right', rotation_mode='anchor')

    if sys.argv[2] == "gap":
        y_min_limit = .8
        step = .1
        ax.set_ylim((y_min_limit, y_limit))
        ax.set_yticks([i for i in np.arange(y_min_limit, y_limit + .01, step)])
        ax.set_yticks([i for i in np.arange(y_min_limit, y_limit + .01, step/2)], minor=True)
    else:
        y_min_limit = .7
        step = .1
        ax.set_ylim((y_min_limit, y_limit))
        ax.set_yticks([i for i in np.arange(y_min_limit, y_limit + .01, step)])
        ax.set_yticks([i for i in np.arange(y_min_limit, y_limit + .01, step/2)], minor=True)

    ax.set_ylabel("Speedup")

    legend = plt.legend(loc=10, bbox_to_anchor=(0.5, 1.2),
          ncol=5, edgecolor='black', framealpha=1.0)
    legend.get_frame().set_alpha(None)
    legend.get_frame().set_facecolor((0, 0, 0, 0))

    # Horizontal line
    plt.axhline(y=1, color='black', linestyle='-', linewidth=1.5)

    plt.gca().yaxis.grid(True, zorder=1, which='major')
    plt.gca().yaxis.grid(True, zorder=1, which='minor', linestyle='--')

    fig.tight_layout()

    plt.savefig("fig9a.pdf", bbox_inches="tight")
    sys.exit()
