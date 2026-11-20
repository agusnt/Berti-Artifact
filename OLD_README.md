# Berti-Artifact

[![DOI](https://zenodo.org/badge/518392799.svg)](https://zenodo.org/badge/latestdoi/518392799)

Artifact used to evaluate the Berti prefetcher presented in the following paper:
>*Agustín Navarro-Torres, Biswabandan Panda, Jesús Alastruey-Benedé, Pablo Ibáñez 
>Víctor Viñals-Yúfera, Alberto Ros*
>"**Berti: an Accurate and Timely Local-Delta Data Prefetcher**".
>To appear in _55th Proceedings of the International Symposium on 
Microarchitecture_ (MICRO-55), October 2022.

## Overview

This repository provides an artifact to reproduce the SPEC CPU2017 single-thread
experiments. In particular it reproduce the fig 8, 9a and 10 from **Berti: an
Accurate and Timely Local-Delta Data Prefetcher**

A full run of the artifact using the parallel flag takes about 1 hour 40 minutes
in an AMD Rome 7702P with 128 threads.

## Tested Environment & Dependencies

- Ubuntu 18.04.6 LTS
- Linux Kernel 5.4.0
- Python 3.6.9
- Bash 4.4.20
- GCC 7.5.0
- Curl 7.61

### Python Packages

Python3 packages needed to generate results and graphs:

- matplotlib 3.3.4
- pprint 0.1
- numpy 1.19.1
- scipy 1.5.4

## Directory Structure

This repository is organized as follow:
- `./Python/`: Python scripts to get the results and graphs
- `./ChampSim/Berti`: Berti code
- `./ChampSim/Other_PF`: Other prefetchers code
- `./compile_gcc.sh`: Script for download and build GCC 7.5.0 from scratch
- `./download_spec2k17.sh`: Script for download SPEC CPU2017 traces

## How to execute the artifact?

The full artifact can be executed typing:

```Bash
./run.sh
```

The available options can be consulted with the following command:

```Bash
./run.sh -h
```

### Sequential vs Parallel execution

To speedup the execution of the artifact we provide an optional flag that runs
the simulations in parallel: `./run.sh -p [number of thread]`.

### Native Run

By default our artifact uses the GCC compiler installed on the system. However,
GCC higher than version 7.5.0 can generate issues when running our ChampSim
version. In order to facilitate the use of our artifact `run.sh` has two
optional options: (1) `./run.sh -g` compiles GCC 7.5.0 from scratch, and (2)
`run.sh -d` which uses *Docker* to compile the artifact.

## Expected output 

After the execution of the artifact an output like this is expected:

```Bash
./run.sh -d -p 127                                                                                                        

Building with Docker
Running in Parallel

Download SPEC CPU2017 traces [44/44]
Building Berti... done
Building MLOP... done
Building IPCP... done
Building IP Stride... done
Making everything ready to run... done
Running... done

Results, it requires numpy, scipy, maptlotlib and pprint

Parsing data... done
SPEC CPU2017 Memory Intensive SpeedUp
--------------------------------------
| Prefetch | Speedup | L1D Accuracy |
| IPCP     | 09%     | 64.9%        |
| MLOP     | 08%     | 68.0%        |
| Berti    | 12%     | 88.0%        |
--------------------------------------
Generating Figure 8 SPEC_CPU2017-MemInt... done
Generating Figure 9 (a)... done
Generating Figure 10 SPEC_CPU2017-MemInt... done
Removing Temporal Files... done
```

## Parameters

```
Options: 
 -h: help
 -v: verbose mode
 -p [num]: run using [num] threads
 -g: build GCC7.5 from scratch
 -d: compile with docker
 -c: clean all generated files (traces and gcc7.5)
 -l: generate a log for debug purpose
 -r: always download SPEC CPU2K17 traces
 -n: no build the simulator
 -f: execute GAP, CloudSuite, and Multi-Level prefetcher
 -m: execute 4-Core
```

## Full Simulation

We have included two extra options that increase the execution time of the
artifact, but in return, it reproduce the results for GAP, CloudSuite, Multi-Level
prefetchers and MultiCore.

To reproduce the results first of all you have to download all the traces. Since
CloudSuite and GAP traces can not be downloaded automatic, so you need to
download [GAP
traces](https://utexas.app.box.com/s/2k54kp8zvrqdfaa8cdhfquvcxwh7yn85/folder/132804668078)
and copy them into `traces/gap` folder. And download [CloudSuite
traces](https://www.dropbox.com/sh/pgmnzfr3hurlutq/AACciuebRwSAOzhJkmj5SEXBa/CRC2_trace?dl=0&subfolder_nav_tracking=1)
and copy them into `traces/cs`.

After that you have to execute `./run.sh -f -m` to run the full abstract. New
pdfs will appear, the name of the pdf corresponds to the figure reproduced in
the article.

**NOTE**: The full simulation options (`-f -m`) takes several days to finish
even in a high performance system.
