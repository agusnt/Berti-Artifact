#!/usr/bin/python3
import parse
import scipy.stats
import numpy as np
import sys
import statistics as stats
from pprint import pprint

spec2k17_memory_intensive = [
            '602.gcc_s-734B.champsimtrace.xz',
            '602.gcc_s-1850B.champsimtrace.xz',
            '602.gcc_s-2226B.champsimtrace.xz',
            '603.bwaves_s-891B.champsimtrace.xz',
            '603.bwaves_s-1740B.champsimtrace.xz',
            '603.bwaves_s-2609B.champsimtrace.xz',
            '603.bwaves_s-2931B.champsimtrace.xz',
            '605.mcf_s-472B.champsimtrace.xz',
            '605.mcf_s-484B.champsimtrace.xz',
            '605.mcf_s-665B.champsimtrace.xz',
            '605.mcf_s-782B.champsimtrace.xz',
            '605.mcf_s-994B.champsimtrace.xz',
            '605.mcf_s-1152B.champsimtrace.xz',
            '605.mcf_s-1536B.champsimtrace.xz',
            '605.mcf_s-1554B.champsimtrace.xz',
            '605.mcf_s-1644B.champsimtrace.xz',
            '607.cactuBSSN_s-2421B.champsimtrace.xz',
            '607.cactuBSSN_s-3477B.champsimtrace.xz',
            '607.cactuBSSN_s-4004B.champsimtrace.xz',
            '619.lbm_s-2676B.champsimtrace.xz',
            '619.lbm_s-2677B.champsimtrace.xz',
            '619.lbm_s-3766B.champsimtrace.xz',
            '619.lbm_s-4268B.champsimtrace.xz',
            '620.omnetpp_s-141B.champsimtrace.xz',
            '620.omnetpp_s-874B.champsimtrace.xz',
            '621.wrf_s-6673B.champsimtrace.xz',
            '621.wrf_s-8065B.champsimtrace.xz',
            '623.xalancbmk_s-10B.champsimtrace.xz',
            '623.xalancbmk_s-165B.champsimtrace.xz',
            '623.xalancbmk_s-202B.champsimtrace.xz',
            '623.xalancbmk_s-325B.champsimtrace.xz',
            '623.xalancbmk_s-592B.champsimtrace.xz',
            '623.xalancbmk_s-700B.champsimtrace.xz',
            '627.cam4_s-490B.champsimtrace.xz',
            '628.pop2_s-17B.champsimtrace.xz',
            '649.fotonik3d_s-1176B.champsimtrace.xz',
            '649.fotonik3d_s-7084B.champsimtrace.xz',
            '649.fotonik3d_s-8225B.champsimtrace.xz',
            '649.fotonik3d_s-10881B.champsimtrace.xz',
            '654.roms_s-293B.champsimtrace.xz',
            '654.roms_s-294B.champsimtrace.xz',
            '654.roms_s-523B.champsimtrace.xz',
            '654.roms_s-1070B.champsimtrace.xz',
            '654.roms_s-1390B.champsimtrace.xz',
            'bc-0.trace.gz',
            'bc-3.trace.gz',
            'bc-5.trace.gz',
            'bc-12.trace.gz',
            'bfs-3.trace.gz',
            'bfs-8.trace.gz',
            'bfs-10.trace.gz',
            'bfs-14.trace.gz',
            'cc-5.trace.gz',
            'cc-6.trace.gz',
            'cc-13.trace.gz',
            'cc-14.trace.gz',
            'pr-3.trace.gz',
            'pr-5.trace.gz',
            'pr-10.trace.gz',
            'pr-14.trace.gz',
            'sssp-3.trace.gz',
            'sssp-5.trace.gz',
            'sssp-10.trace.gz',
            'sssp-14.trace.gz'
        ]

only_intensive = False

def add_to_dic(dic, value, i, key):
    # Pre-buscadores
    l1ipref = i.split('-')[1]
    l1dpref = i.split('-')[2]
    l2dpref = i.split('-')[3]
    perfect = i.split('-')[17]
    bench = i.split('---')[-1]

    if only_intensive and bench not in spec2k17_memory_intensive:
        return

    # Casos especiales (cache perfecta, tamaños de biswa)
    if perfect != 'no':
        aux = perfect.split('_')[0]
        # Cache Perfecta 
        if aux == 'PERFECT': 
            l1dpref = '_'.join(perfect.split('_')[0:2])
            l2dpref = '_'.join(perfect.split('_')[2:])
        elif aux == 'PRACTICAL': 
            l1dpref = '_'.join(perfect.split('_')[0:3])
            l2dpref = '_'.join(perfect.split('_')[3:])
        else:
            if l1dpref == "ip_stride":
                l1dpref = "{}_{}".format(l1dpref, perfect)
            if l1dpref == 'ipcp_isca2020':
                l1dpref = "{}_{}".format(l1dpref, perfect)
            if l2dpref == 'ipcp_isca2020':
                l2dpref = "{}_{}".format(l2dpref, perfect)
            if l1dpref == "vberti":
                l1dpref = "{}_{}".format(l1dpref, perfect)
            if l1dpref == "vberti_micro":
                l1dpref = "{}_{}".format(l1dpref, perfect)
            if l1dpref == "vberti_ross_hash_launch":
                l1dpref = "{}_{}".format(l1dpref, perfect)
            if l1dpref == "vberti_ross_hash":
                l1dpref = "{}_{}".format(l1dpref, perfect)
    
    # Crear estructura
    if l1ipref not in dic:
        dic[l1ipref] = {}
    if l1dpref not in dic[l1ipref]:
        dic[l1ipref][l1dpref] = {}
    if l2dpref not in dic[l1ipref][l1dpref]:
        dic[l1ipref][l1dpref][l2dpref] = {}
    if bench not in dic[l1ipref][l1dpref][l2dpref]:
        dic[l1ipref][l1dpref][l2dpref][bench] = {}

    dic[l1ipref][l1dpref][l2dpref][bench][key] = value

def get_ipc(fname, dic):
    # Obtener datos
    raw = parse.iterateOverDir(fname, "CPU [0-9] cumulative IPC:.*")

    for i in raw:
        # Simple comprobacion de errores
        if (len(raw) < 1):
            print("Error: {}".format(i))
            sys.exit()
        for ii in raw[i]:
            if ii == []:
                print("Error: {}".format(i))
                sys.exit()

        # Get the data
        aux = []
        for ii in raw[i]:
            for iii in ii:
                aux.append(float(iii.split('IPC: ')[-1].split(' ')[0]))
        
        if aux == []:
            continue

        if len(aux) > 1:
            # Simulaciones Multicore
            ipc = stats.harmonic_mean(aux)
        else:
            ipc = aux[0]

        add_to_dic(dic, ipc, i, "ipc")

    # Calculate SpeedUp    
    for i in dic:
        for ii in dic[i]:
            for iii in dic[i][ii]:
                for iiii in dic[i][ii][iii]:
                    if i == 'no' and ii == 'ip_stride_8_ways' \
                            and iii == 'no':
                        dic[i][ii][iii][iiii]['SpeedUp'] = 1
                    else:
                        aux = dic[i][ii][iii][iiii]['ipc'] /\
                            dic['no']['ip_stride']['no'][iiii]['ipc']

                        dic[i][ii][iii][iiii]['SpeedUp'] = aux

def get_l1daccuracy(fname, dic):
    # Obtener datos
    raw = parse.iterateOverDir(fname, "L1D USEFUL LOAD PREFETCHES.*")
    raw_2 = parse.iterateOverDir(fname, "L1D PREFETCH  REQUESTED.*")
    raw_3 = parse.iterateOverDir(fname, "L1D TIMELY PREFETCHES.*")

    for i, j, k in zip(raw, raw_2, raw_3):
        # Simple comprobacion de errores
        if (len(raw) < 1):
            print("Error: {}".format(i))
            sys.exit()
        for ii in raw[i]:
            if ii == []:
                print("Error: {}".format(i))
                sys.exit()

        time = []
        late = []
        for ii, jj, kk in zip(raw[i], raw_2[j], raw_3[k]):
            for iii, jjj, kkk in zip(ii, jj, kk):

                useful = (int(' '.join(kkk.split()).split('TIMELY PREFETCHES: ')[-1].split(' ')[0]))
                latepf = (int(' '.join(kkk.split()).split('LATE PREFETCHES: ')[-1].split(' ')[0]))
                issued = (int(' '.join(jjj.split()).split('USELESS: ')[-1].split(' ')[0]))


                total = useful + latepf + issued

                if issued != 0:
                    late.append((latepf + useful) / total)
                    time.append(useful / total)
        
        if time == []:
            continue

        if len(time) > 1:
            # Simulaciones Multicore
            timepf = np.mean(time)
            latepf = np.mean(late)
        else:
            timepf = time[0]
            latepf = late[0]
        if time != time:
            continue

        add_to_dic(dic, timepf, i, "L1DAccuracyTime")
        add_to_dic(dic, latepf, i, "L1DAccuracyLate")

def get_l1dcoverage(fname, dic):
    raw = parse.iterateOverDir(fname, "L1D TIMELY PREFETCHES:.*")
    raw_2 = parse.iterateOverDir(fname, "L1D LOAD.*")
    raw_3 = parse.iterateOverDir(fname, "L1D RFO.*")

    for i, j, k in zip(raw, raw_2, raw_3):
        # Simple comprobacion de errores
        if (len(raw) < 1):
            print("Error: {}".format(i))
            sys.exit()
        for ii in raw[i]:
            if ii == []:
                print("Error: {}".format(i))
                sys.exit()

        
        time = []
        late = []
        for ii, jj, kk in zip(raw[i], raw_2[j], raw_3[k]):
            for iii, jjj, kkk in zip(ii, jj, kk):
                aux_pf = (int(' '.join(iii.split()).split('TIMELY PREFETCHES: ')[-1].split(' ')[0]))
                aux_la = (int(' '.join(iii.split()).split('LATE PREFETCHES: ')[-1].split(' ')[0]))
                aux_mi = (
                    (int(' '.join(jjj.split()).split('MISS: ')[-1].split(' ')[0]))
                    +
                    (int(' '.join(kkk.split()).split('MISS: ')[-1].split(' ')[0]))
                    )

                if (aux_pf + aux_mi) != 0:
                    time.append(((aux_pf / (aux_pf + aux_mi))))
                    late.append((((aux_pf + aux_la) / (aux_pf + aux_mi))))
        
        if time == []:
            continue

        if len(time) > 1:
            # Simulaciones Multicore
            time = np.mean(time)
            late = np.mean(late)
        else:
            time = time[0]
            late = late[0]

        if late != late:
            continue

        add_to_dic(dic, time, i, "L1DCoverageTime")
        add_to_dic(dic, late, i, "L1DCoverageLate")

def get_l2ccoverage(fname, dic):
    raw = parse.iterateOverDir(fname, "L2C LOAD      ACCESS.*")
    raw_2 = parse.iterateOverDir(fname, "L2C RFO.*")
    raw_3 = parse.iterateOverDir(fname, "L2C TIMELY PREFETCHES:.*")

    for i, j, k in zip(raw, raw_2, raw_3):
        # Simple comprobacion de errores
        miss = []
        lmiss = []

        if (raw[i] == [[]]):
            raw[i] = [["L2C TOTAL     ACCESS:   MISS: 0"]]
        if (raw_2[j] == [[]]):
            raw_2[j] = [["L2C RFO:   MISS: 0"]]
        if (raw_3[j] == [[]]):
            raw_3[j] = [["L2C TIMELY PREFETCHES:  LATE PREFETCHES: 0"]]

        for ii, jj, kk in zip(raw[i], raw_2[j], raw_3[k]):
            for iii, jjj, kkk in zip(ii, jj, kk):
                misses = (
                    (int(' '.join(iii.split()).split('MISS: ')[-1].split(' ')[0]))
                    +
                    (int(' '.join(jjj.split()).split('MISS: ')[-1].split(' ')[0]))
                    )
                late = (int(' '.join(kkk.split()).split('LATE PREFETCHES: ')[-1].split(' ')[0]))

                miss.append(misses)
                lmiss.append(misses - late)
        
        if len(miss) > 1:
            # Simulaciones Multicore
            miss = np.mean(miss)
            lmiss = np.mean(lmiss)
        else:
            miss = miss[0]
            lmiss = lmiss[0]

        if miss != miss:
            continue

        add_to_dic(dic, miss, i, "L2CMiss")
        add_to_dic(dic, miss, i, "L2CLMiss")

    # Coverage
    for i in dic:
        for ii in dic[i]:
            for iii in dic[i][ii]:
                for iiii in dic[i][ii][iii]:
                    if i == 'no' and ii == 'no' and iii == 'no':
                        dic[i][ii][iii][iiii]['L2CCoverage'] = 0
                    else:
                        if dic['no']['no']['no'][iiii]['L2CMiss'] == 0:
                            continue

                        aux = 1 - (dic[i][ii][iii][iiii]['L2CMiss'] /\
                            dic['no']['no']['no'][iiii]['L2CMiss'])

                        dic[i][ii][iii][iiii]['L2CCoverageTime'] = aux

                        aux = 1 - (dic[i][ii][iii][iiii]['L2CLMiss'] /\
                            dic['no']['no']['no'][iiii]['L2CLMiss'])

                        dic[i][ii][iii][iiii]['L2CCoverageLate'] = aux

if __name__ == "__main__":
    dic = {}
    base = {}
    if sys.argv[1] == "y":
        # Only memory intensive SPEC2K17
        only_intensive = True

    if sys.argv[2] == "SpeedUp":
        get_ipc(sys.argv[3], dic)
    if sys.argv[2] == "L1DAccuracyTime" or sys.argv[2] == "L1DAccuracyLate":
        get_l1daccuracy(sys.argv[3], dic)
    if sys.argv[2] == "L1DCoverageTime" or sys.argv[2] == "L1DCoverageLate":
        get_l1dcoverage(sys.argv[3], dic)
    if sys.argv[2] == "L2CCoverageTime" or sys.argv[2] == "L2CCoverageLate":
        get_l2ccoverage(sys.argv[3], dic)

    # Print head
    head = "Bench;{}".format(sys.argv[2])
    subhead = ";"
    print(head)

    # Print all lines
    line = {}
    for i in dic:
        # L1I PF level
        for ii in sorted(dic[i]):
            # L1D PF level
            for iii in sorted(dic[i][ii]):
                # L2C PF Level
                subhead = "{}{};".format(subhead, "{}_{}".format(ii, iii))

                aux = []
                for iiii in sorted(dic[i][ii][iii]):
                    if sys.argv[2] not in dic[i][ii][iii][iiii]:
                        line[iiii] = "{};".format(iiii)
                        continue
                    if iiii not in line:
                        line[iiii] = "{};".format(iiii)

                    line[iiii] = "{}{};".format(line[iiii], 
                            dic[i][ii][iii][iiii][sys.argv[2]])

    print(subhead)
    for i in sorted(line):
        print(line[i])
