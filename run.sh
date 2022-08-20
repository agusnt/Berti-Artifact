#!/bin/bash

#
# Run the Berti's artifact
#
# @Author: Navarro Torres, Agustín
# @Date: 25/07/2022
#

################################################################################
#                            Configuration Vars                              #
################################################################################
VERBOSE=""
PARALLEL=""
GCC="N"
NUM_THREAD="1"
DOCKER="N"
REMOVE_ALL="N"
LOGGED="N"
DOWNLOAD="N"
BUILD="Y"
FULL="N"
MULTI="N"

################################################################################
#                                Global Vars                                 #
################################################################################

CONTAINER="docker"
DIR=$(pwd)
BERTI="./ChampSim/Berti"
PF="./ChampSim/Other_PF"
TRACES_SPEC="traces/spec2k17"
TRACES_GAP="traces/gap"
TRACES_CS="traces/cs"
OUT_BASE="output"
OUT="output"
LOG=$(pwd)/stderr.log

################################################################################
#                                Terminal Colors                             #
################################################################################

# Terminal colors
GREEN=$'\e[0;32m'
RED=$'\e[0;31m'
NC=$'\e[0m'

################################################################################
#                           Auxiliary functions                              #
################################################################################
run_command () 
{
    # Run command 
    if [[ "$VERBOSE" == "Y" ]]; then
        # Without stdout/stderr redirect
        $1
    elif [[ "$LOGGED" == "Y" ]]; then
        # Log
        $1 >> $LOG 2>&1
    else
        # With stdout/stderr redirect
        $1 >/dev/null 2>&1
    fi
    
    # Command error
    if [ $? -ne 0 ]; then
        echo " ${RED}ERROR${NC}"
        exit
    fi
    echo " ${GREEN}done${NC}"
}

file_trace () 
{
    # Generate temporal files to run simulations in parallel
    for i in $2/*;
    do
        trace=$(echo $i | rev | cut -d'/' -f1 | rev)
        if [[ "$LOGGED" == "Y" ]]; then
            echo -n "$1 -warmup_instructions 50000000 -simulation_instructions"
            echo " 200000000 -traces $i > $OUT/$3---$trace 2>>$LOG"
        else
            echo -n "$1 -warmup_instructions 50000000 -simulation_instructions"
            echo " 200000000 -traces $i > $OUT/$3---$trace 2>/dev/null"
        fi
    done
}

file_4core_trace () 
{
    idx=0
    >&2 echo $1
    # Generate temporal files to run simulations in parallel
    while read -r line
    do
        trace="$trace $line"
        if [[ ! -z $line ]]; then
            continue
        fi

        if [[ "$LOGGED" == "Y" ]]; then
            echo -n "$1 -warmup_instructions 50000000 -simulation_instructions"
            echo " 200000000 -traces $trace > $OUT/$3.out---$idx.out 2>>$LOG"
        else
            echo -n "$1 -warmup_instructions 50000000 -simulation_instructions"
            echo " 200000000 -traces $trace > $OUT/$3.out---$idx.out 2>/dev/null"
        fi
        idx=$(($idx + 1))
        trace=""
    done < $2
}

run_compile ()
{
    # Build ChampSim with the given prefetcher
    if [[ "$GCC" == "Y" ]]; then
        # Use GCC building from scratch
        run_command "$1 $CCX"
    elif [[ "$DOCKER" == "Y" ]]; then
        # Use Docker GCC
        if [[ "$VERBOSE" == "Y" ]]; then
            if ! command -v getenforce &> /dev/null
            then
                # System without SELinux
                $CONTAINER run -it -v$(pwd):/mnt --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1"
            else
                # System wit SELinux
                $CONTAINER run -it -v$(pwd):/mnt:Z --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1"
            fi
        elif [[ "$LOGGED" == "Y" ]]; then
            if ! command -v getenforce &> /dev/null
            then
                # System without SELinux
                $CONTAINER run -it -v$(pwd):/mnt --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1" >> $LOG 2>&1
            else
                # System with SELinux
                $CONTAINER run -it -v$(pwd):/mnt:Z --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1" >> $LOG 2>&1
            fi
        else
            if ! command -v getenforce &> /dev/null
            then
                # System without SELinux
                $CONTAINER run -it -v$(pwd):/mnt --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1" > /dev/null 2>&1
            else
                # System with SELinux
                $CONTAINER run -it -v$(pwd):/mnt:Z --rm gcc:7.5.0 /bin/bash -c "cd mnt; $1" > /dev/null 2>&1
            fi
        fi
        if [ $? -ne 0 ]; then
            echo " ${RED}ERROR${NC}"
            exit
        fi
        echo " ${GREEN}done${NC}"
    else
        # Use system GCC
        run_command "$1"
    fi
}

print_help ()
{
    echo "Run Berti Artificat"
    echo "Options: "
    echo " -h: help"
    echo " -v: verbose mode"
    echo " -p [num]: run using [num] threads"
    echo " -g: build GCC7.5 from scratch"
    echo " -d: compile with docker" 
    echo " -c: clean all generated files (traces and gcc7.5)" 
    echo " -l: generate a log for debug purpose" 
    echo " -r: always download SPEC CPU2K17 traces" 
    echo " -n: no build the simulator" 
    echo " -f: execute GAP, CloudSuite, and Multi-Level prefetcher" 
    echo " -m: execute 4-Core" 
    exit
}
################################################################################
#                                Parse Options                               #
################################################################################

while getopts :mfvlrcdhngp: opt; do
    case "${opt}" in
          v) VERBOSE="Y"
              echo -e "\033[1mVerbose Mode\033[0m"
              ;;
          l) LOGGED="Y"
              echo -e "\033[1mLog Mode\033[0m"
              echo -n "" > $LOG
              ;;
          g) GCC="Y"
              echo -e "\033[1mDownloading and Building with GCC 7.5\033[0m"
              ;;
          p) PARALLEL="Y"
              NUM_THREAD=${OPTARG}
              echo -e "\033[1mRunning in Parallel\033[0m"
              ;;
          d) DOCKER="Y"
              echo -e "\033[1mBuilding with Docker\033[0m"
              ;;
          c) REMOVE_ALL="Y"
              echo -e "\033[1m${RED}REMOVING ALL TEMPORAL FILES (traces and gcc7.5)${NC}\033[0m"
              ;;
          r) DOWNLOAD="Y"
              echo -e "\033[1mAlways download SPEC CPU2K17 traces\033[0m"
              ;;
          n) BUILD="N"
              echo -e "\033[1mNOT build the simulator\033[0m"
              ;;
          f) FULL="Y"
              echo -e "\033[1mFull execution\033[0m"
              ;;
          m) MULTI="Y"
              echo -e "\033[1mMulti-Core execution\033[0m"
              ;;
          h) print_help;;
     esac
done

################################################################################
#                                Scripts Body                                #
################################################################################

# Just in case, fix execution permission
chmod +x Python/*.py
chmod +x *.sh
chmod +x ChampSim/Berti/*.sh
chmod +x ChampSim/Other_PF/*.sh
    
echo ""

# Build GCC 7.5.0 from scratch
if [[ "$GCC" == "Y" ]]; then
    echo -n "Building GCC 7.5 from scratch..."

    if [[ "$VERBOSE" == "Y" ]]; then
        ./compile_gcc.sh $PARALLEL $NUM_THREAD
    elif [[ "$LOGGED" == "Y" ]]; then
        ./compile_gcc.sh $PARALLEL $NUM_THREAD >> $LOG 2>&1
    else
        ./compile_gcc.sh $PARALLEL $NUM_THREAD >/dev/null 2>&1
    fi

    echo " ${GREEN}done${NC}"
    CCX=$(pwd)/gcc7.5/gcc-7.5.0/bin/bin/g++
fi

#----------------------------------------------------------------------------#
#                            Download SPEC2K17 Traces                        #
#----------------------------------------------------------------------------#


if [[ "$LOGGED" == "Y" ]]; then
    echo "DOWNLOAD TRACES" >> $LOG
    echo "============================================================" >> $LOG
fi

if [ ! -d "$TRACES_SPEC" ] || [ "$DOWNLOAD" == "Y" ]; then
    ./download_spec2k17.sh $TRACES_SPEC
fi

#----------------------------------------------------------------------------#
#                                Build ChampSim                              #
#----------------------------------------------------------------------------#
if [[ "$BUILD" == "Y" ]]; then
    if [[ "$LOGGED" == "Y" ]]; then
        echo "Building" >> $LOG
        echo "============================================================" >> $LOG
    fi

    echo -n "Building Berti..."
    cd $BERTI
    run_compile "./build_champsim.sh hashed_perceptron no vberti no no no no no\
            lru lru lru srrip drrip lru lru lru 1 no"
    cd $DIR
    
    # Build MLOP, IPCP and IP Stride
    cd $PF
    echo -n "Building MLOP..."
    run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3 no no no no no\
            lru lru lru srrip drrip lru lru lru 1 no"
    
    echo -n "Building IPCP..."
    run_compile "./build_champsim.sh hashed_perceptron no ipcp_isca2020 no no no no\
            no lru lru lru srrip drrip lru lru lru 1 no"
    
    echo -n "Building IP Stride..."
    run_compile "./build_champsim.sh hashed_perceptron no ip_stride no no no no no\
            lru lru lru srrip drrip lru lru lru 1 no"

    if [[ "$FULL" == "Y" ]]; then
        echo -n "Building No Prefetcher..."
        run_compile "./build_champsim.sh hashed_perceptron no no\
                no no no no no lru lru lru srrip drrip lru lru lru 1 no"
    
        echo -n "Building IPCP+IPCP..."
        run_compile "./build_champsim.sh hashed_perceptron no ipcp_isca2020\
                ipcp_isca2020 no no no no lru lru lru srrip drrip lru lru lru 1 no"

        echo -n "Building MLOP+Bingo..."
        run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3\
                bingo_dpc3 no no no no lru lru lru srrip drrip lru lru lru 1 no"

        echo -n "Building MLOP+SPP-PPF..."
        run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3\
                ppf no no no no lru lru lru srrip drrip lru lru lru 1 no"
        cd $DIR

        cd $BERTI
        echo -n "Building Berti+Bingo..."
        run_compile "./build_champsim.sh hashed_perceptron no vberti\
                bingo_dpc3 no no no no lru lru lru srrip drrip lru lru lru 1 no"

        echo -n "Building Berti+SPP-PPF..."
        run_compile "./build_champsim.sh hashed_perceptron no vberti\
                ppf no no no no lru lru lru srrip drrip lru lru lru 1 no"
    fi

    if [[ "$MULTI" == "Y" ]]; then
        echo -n "Building 4-Core Berti..."
        run_compile "./build_champsim.sh hashed_perceptron no vberti\
                no no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core Berti+Bingo..."
        run_compile "./build_champsim.sh hashed_perceptron no vberti\
                bingo_dpc3 no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core Berti+SPP-PPF..."
        run_compile "./build_champsim.sh hashed_perceptron no vberti\
                ppf no no no no lru lru lru srrip drrip lru lru lru 4 no"
        cd $DIR

        cd $PF
        echo -n "Building 4-Core IPCP..."
        run_compile "./build_champsim.sh hashed_perceptron no ipcp_isca2020\
                no no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core MLOP..."
        run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3\
                no no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core IPCP+IPCP..."
        run_compile "./build_champsim.sh hashed_perceptron no ipcp_isca2020\
                ipcp_isca2020 no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core MLOP+Bingo..."
        run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3\
                bingo_dpc3 no no no no lru lru lru srrip drrip lru lru lru 4 no"

        echo -n "Building 4-Core MLOP+SPP-PPF..."
        run_compile "./build_champsim.sh hashed_perceptron no mlop_dpc3\
                ppf no no no no lru lru lru srrip drrip lru lru lru 4 no"
        cd $DIR
    fi

    cd $DIR
fi

#----------------------------------------------------------------------------#
#                                Running Simulations                         #
#----------------------------------------------------------------------------#
mkdir $OUT > /dev/null 2>&1

if [[ "$LOGGED" == "Y" ]]; then
    echo "RUNNING" >> $LOG
    echo "============================================================" >> $LOG
fi

# Prepare to run in parallel
echo -n "Making everything ready to run..."

echo -n "" > tmp_par.out

for i in $(ls $BERTI/bin/*1core*); do
    if [[ "$LOGGED" == "Y" ]]; then
        echo "$BERTI/bin/$i" >> $LOG
        strings -a $BERTI/bin/$i | grep "GCC: " >> $LOG 2>&1
    fi
    name=$(echo $i | rev | cut -d/ -f1 | rev)

    OUT=$OUT_BASE/spec2k17
    aux=$i
    mkdir $OUT > /dev/null 2>&1
    file_trace $i $TRACES_SPEC $name >> tmp_par.out

    if [[ "$FULL" == "Y" ]]; then
        OUT=$OUT_BASE/gap
        mkdir $OUT > /dev/null 2>&1
        file_trace $aux $TRACES_GAP $name >> tmp_par.out
        OUT=$OUT_BASE/cloudsuite
        mkdir $OUT > /dev/null 2>&1
        file_trace $aux $TRACES_CS $name >> tmp_par.out
    fi
done

for i in $(ls $PF/bin/*1core*); do
    if [[ "$LOGGED" == "Y" ]]; then
        echo "$PF/bin/$i" >> $LOG
        strings -a $i | grep "GCC: " >> $LOG 2>&1
    fi
    name=$(echo $i | rev | cut -d/ -f1 | rev)

    OUT=$OUT_BASE/spec2k17
    aux=$i
    file_trace $i $TRACES_SPEC $name >> tmp_par.out

    if [[ "$FULL" == "Y" ]]; then
        OUT=$OUT_BASE/gap
        file_trace $aux $TRACES_GAP $name >> tmp_par.out
        OUT=$OUT_BASE/cloudsuite
        file_trace $aux $TRACES_CS $name >> tmp_par.out
    fi
done

# Run in parallel

if [[ "$MULTI" == "Y" ]]; then
    for i in $(ls $BERTI/bin/*4core*); do
        if [[ "$LOGGED" == "Y" ]]; then
            echo "$i" >> $LOG
            strings -a $BERTI/bin/$i | grep "GCC: " >> $LOG 2>&1
        fi
        name=$(echo $i | rev | cut -d/ -f1 | rev)
    
        OUT=$OUT_BASE/4core
        mkdir $OUT > /dev/null 2>&1
        file_4core_trace $i 4core.in $name >> tmp_par.out
    done
    
    for i in $(ls $PF/bin/*4core*); do
        if [[ "$LOGGED" == "Y" ]]; then
            echo "$i" >> $LOG
            strings -a $PF/bin/$i | grep "GCC: " >> $LOG 2>&1
        fi
        name=$(echo $i | rev | cut -d/ -f1 | rev)
    
        OUT=$OUT_BASE/4core
        mkdir $OUT > /dev/null 2>&1
        file_4core_trace $i 4core.in 4core >> tmp_par.out
    done
    
fi
echo " ${GREEN}done${NC}"

echo -n "Running..."
cat tmp_par.out | xargs -I CMD -P $NUM_THREAD bash -c CMD
echo " ${GREEN}done${NC}"

#----------------------------------------------------------------------------#
#                                Generate Results                            #
#----------------------------------------------------------------------------#
if [[ "$LOGGED" == "Y" ]]; then
    echo "Results" >> $LOG
    echo "============================================================" >> $LOG
fi

echo ""
echo -e "\033[1mResults, it requires numpy, scipy, maptlotlib and pprint\033[0m"
echo ""
echo -n "SPEC CPU2K17 Parsing data..."
if [[ "$VERBOSE" == "Y" ]]; then
    python3 Python/get_data.py y output/spec2k17 > single.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data.py y output/spec2k17 > single.csv 2>>$LOG
else
    python3 Python/get_data.py y output/spec2k17 > single.csv 2>/dev/null
fi
echo " ${GREEN}done${NC}"

echo "SPEC CPU2K17 Memory Intensive SpeedUp"
echo "--------------------------------------"
echo "| Prefetch | Speedup | L1D Accuracy |"
while read line; do 
    speed=$(echo $line | cut -d';' -f3)
    accur=$(echo $line | cut -d';' -f5)
    if [[ $(echo $line | cut -d';' -f1) == "vberti" ]]; then
        echo "| Berti    | $speed%     | $accur%        |"
    elif [[ $(echo $line | cut -d';' -f1) == "ipcp_isca2020" ]]; then
        echo "| IPCP     | $speed%     | $accur%        |"
    elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]]; then
        echo "| MLOP     | $speed%     | $accur%        |"
    fi
done < single.csv
echo "--------------------------------------"

echo -n "Generating Figure 8 SPEC17-MemInt..."
echo "spec2k17_memint" > single.csv
if [[ "$VERBOSE" == "Y" ]]; then
    python3 Python/get_data_fig.py y output/spec2k17 >> single.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>>$LOG
else
    python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>/dev/null
fi
run_command "python3 Python/one_prefetch_performance.py single.csv"

echo -n "Generating Figure 9 (a)..."
if [[ "$VERBOSE" == "Y" ]]; then
    python3 Python/get_data_by_traces.py y SpeedUp output/spec2k17 > spec.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data_by_traces.py y SpeedUp output/spec2k17 > spec.csv 2>>$LOG
else
    python3 Python/get_data_by_traces.py y SpeedUp output/spec2k17 > spec.csv 2>/dev/null
fi
run_command "python3 Python/by_app_performance.py spec.csv cpu"

echo -n "Generating Figure 10 SPEC17-MemInt..."
run_command "python3 Python/l1d_accuracy.py single.csv"

if [[ "$FULL" == "Y" ]]; then
#----------------------------------------------------------------------------#
#                                     GAP                                      #
#----------------------------------------------------------------------------#
    echo -n "GAP Parsing data..."
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data.py n output/gap > single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data.py n output/gap > single.csv 2>>$LOG
    else
        python3 Python/get_data.py n output/gap > single.csv 2>/dev/null
    fi
    echo " ${GREEN}done${NC}"
    
    echo "GAP Memory Intensive L1D Prefetcher SpeedUp"
    echo "--------------------------------------"
    echo "| Prefetch | Speedup | L1D Accuracy |"
    while read line; do 
        speed=$(echo $line | cut -d';' -f3)
        accur=$(echo $line | cut -d';' -f5)
        if [[ $(echo $line | cut -d';' -f1) == "vberti" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "no" ]]; then
            echo "| Berti    | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "ipcp_isca2020" ]] &&
            [[ $(echo $line | cut -d';' -f2) == "no" ]]; then
            echo "| IPCP     | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "no" ]]; then
            echo "| MLOP     | $speed%     | $accur%        |"
        fi
    done < single.csv
    echo "--------------------------------------"
    
    echo -n "Generating Figure 8 GAP..."
    echo "gap" > single.csv
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>/dev/null
    fi
    run_command "python3 Python/one_prefetch_performance_gap.py single.csv"
    mv fig8.pdf fig8_gap.pdf
    
    echo -n "Generating Figure 9 (b)..."
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_by_traces.py n SpeedUp output/gap > spec.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_by_traces.py n SpeedUp output/gap > spec.csv 2>>$LOG
    else
        python3 Python/get_data_by_traces.py n SpeedUp output/gap > spec.csv 2>/dev/null
    fi
    run_command "python3 Python/by_app_performance.py spec.csv gap"
    mv fig9a.pdf fig9b.pdf
    
    echo -n "Generating Figure 10 GAP..."
    run_command "python3 Python/l1d_accuracy_gap.py single.csv"
    mv fig10.pdf fig10_gap.pdf

#----------------------------------------------------------------------------#
#                              Coverage                                      #
#----------------------------------------------------------------------------#
    echo -n "Generating Figure 11..."
    echo "spec2k17_intensive" > single.csv
     if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>/dev/null
    fi

    echo "gap" >> single.csv
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>/dev/null
    fi
    run_command "python3 Python/mpki_by_suite.py single.csv"
 
#----------------------------------------------------------------------------#
#                              Multi-Level                                     #
#----------------------------------------------------------------------------#
    echo -n "SPEC CPU2K17 Parsing data for Multi-Level prefetching..."
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data.py y output/spec2k17 > single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data.py y output/spec2k17 > single.csv 2>>$LOG
    else
        python3 Python/get_data.py y output/spec2k17 > single.csv 2>/dev/null
    fi
    echo " ${GREEN}done${NC}"
    
    echo "SPEC CPU2K17 Memory Intensive Multi-Level SpeedUp"
    echo "-------------------------------------------"
    echo "| Prefetch       | Speedup | L1D Accuracy |"
    while read line; do 
        speed=$(echo $line | cut -d';' -f3)
        accur=$(echo $line | cut -d';' -f5)
        if [[ $(echo $line | cut -d';' -f1) == "vberti" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "bingo_dpc3" ]]; then
            echo "| Berti+Bingo    | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "vberti" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "ppf" ]]; then
            echo "| Berti+SPP-PPF  | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "ipcp_isca2020" ]] &&
            [[ $(echo $line | cut -d';' -f2) == "ipcp_isca2020" ]]; then
            echo "| IPCP+IPCP      | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "bingo_dpc3" ]]; then
            echo "| MLOP+Bingo     | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "ppf" ]]; then
            echo "| MLOP+SPP-PPF   | $speed%     | $accur%        |"
        fi
    done < single.csv
    echo "-------------------------------------------"
    
    echo -n "GAP Parsing data for Multi-Level prefetching..."
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data.py n output/gap > single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data.py n output/gap > single.csv 2>>$LOG
    else
        python3 Python/get_data.py n output/gap > single.csv 2>/dev/null
    fi
    echo " ${GREEN}done${NC}"
    
    echo "GAP Memory Intensive Multi-Level SpeedUp"
    echo "-------------------------------------------"
    echo "| Prefetch       | Speedup | L1D Accuracy |"
    while read line; do 
        speed=$(echo $line | cut -d';' -f3)
        accur=$(echo $line | cut -d';' -f5)
        if [[ $(echo $line | cut -d';' -f1) == "vberti" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "bingo_dpc3" ]]; then
            echo "| Berti+Bingo    | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "vberti" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "ppf" ]]; then
            echo "| Berti+SPP-PPF  | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "ipcp_isca2020" ]] &&
            [[ $(echo $line | cut -d';' -f2) == "ipcp_isca2020" ]]; then
            echo "| IPCP+IPCP      | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "bingo_dpc3" ]]; then
            echo "| MLOP+Bingo     | $speed%     | $accur%        |"
        elif [[ $(echo $line | cut -d';' -f1) == "mlop_dpc3" ]] && 
            [[ $(echo $line | cut -d';' -f2) == "ppf" ]]; then
            echo "| MLOP+SPP-PPF   | $speed%     | $accur%        |"
        fi
    done < single.csv
    echo "-------------------------------------------"
    
    echo -n "Generating Figure 12..."
    echo "spec2k17_memint" > single.csv
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>/dev/null
    fi
    echo "gap" >> single.csv
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>/dev/null
    fi
    run_command "python3 Python/two_prefetch_performance.py single.csv"

    echo -n "Generating Figure 13..."
    echo "spec2k17_intensive" > single.csv
     if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py y output/spec2k17 >> single.csv 2>/dev/null
    fi

    echo "gap" >> single.csv
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>>$LOG
    else
        python3 Python/get_data_fig.py n output/gap >> single.csv 2>/dev/null
    fi
    run_command "python3 Python/mpki_by_suite.py single.csv"


#----------------------------------------------------------------------------#
#                                  CloudSuite                                  #
#----------------------------------------------------------------------------#
    echo -n "Removing unicode from outputs... "
        ./format_cloudsuite.sh output/cs
    echo " ${GREEN}done${NC}"
    
    echo -n "Parsing CloudSuite... "
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_by_traces.py n SpeedUp output/cs > single.csv
        python3 Python/parse_cs.py single.csv > cs.csv
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_by_traces.py n SpeedUp output/cs > single.csv 2>>$LOG
        python3 Python/parse_cs.py single.csv > cs.csv 2>>$LOG
    else
        python3 Python/get_data_by_traces.py n SpeedUp output/cs > single.csv 2>/dev/null
        python3 Python/parse_cs.py single.csv > cs.csv 2>/dev/null
    fi
    echo " ${GREEN}done${NC}"
    
    echo -n "Generating Figure 18... "
    run_command "python3 Python/fig_cs.py cs.csv"
fi

#----------------------------------------------------------------------------#
#                                     4Core                                    #
#----------------------------------------------------------------------------#
if [[ "$MULTI" == "Y" ]]; then
    echo -n "Parsing 4-Core Simulations... "
    if [[ "$VERBOSE" == "Y" ]]; then
        python3 Python/get_data_4core.py n output/4core > 4core.out
    elif [[ "$LOGGED" == "Y" ]]; then
        python3 Python/get_data_4core.py n output/4core > 4core.out 2>>$LOG
    else
        python3 Python/get_data_4core.py n output/4core > 4core.out 2>/dev/null
    fi
    echo " ${GREEN}done${NC}"
    
    echo -n "Generating Figure 20... "
    run_command "python3 Python/multi_performance.py 4core.out"
    mv multi_performance-rebuttal.pdf fig20.pdf
    
    echo -n "Removing Temporal Files..."
    run_command "rm single.csv spec.csv tmp_par.out 4core.out"
    
    if [[ "$REMOVE_ALL" == "Y" ]]; then
        echo -n "Removing All Files..."
        run_command "rm -rf output traces gcc7.5 ChampSim/Berti/bin ChampSim/Other_PF/bin"
    fi
fi
