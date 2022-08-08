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
NUM_THREADS=""
DOCKER="N"
REMOVE_ALL="N"
LOGGED="N"
DOWNLOAD="N"

################################################################################
#                                Global Vars                                 #
################################################################################

CONTAINER="podman"
DIR=$(pwd)
BERTI="./ChampSim/Berti"
PF="./ChampSim/Other_PF"
TRACES_SPEC="traces/spec2k17"
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

run_trace () 
{
    # Run traces sequentially
    NUM_TRACES=$(ls $2/* | wc -l)
    num=0

    echo -n -e "\rRunning $4 with SPEC2K17 traces [0/$NUM_TRACES]"
    for i in $2/*;
    do
        trace=$(echo $i | rev | cut -d'/' -f1 | rev)

        if [[ "$LOGGED" == "Y" ]]; then
            # Log
            $1 -warmup_instructions 50000000 -simulation_instructions 200000000\
                -traces $i > $OUT/$3---$trace 2>> $LOG
        else
            $1 -warmup_instructions 50000000 -simulation_instructions 200000000\
                -traces $i > $OUT/$3---$trace 2>/dev/null
        fi
        num=$(($num + 1))
        echo -n -e "\rRunning $4 with SPEC2K17 traces [$num/$NUM_TRACES]"
    done
    echo -e "\rRunning $4 with SPEC2K17 traces ${GREEN}[$num/$NUM_TRACES]${NC}"
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
    exit
}
################################################################################
#                                Parse Options                               #
################################################################################

while getopts :vlrcdhgp: opt; do
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
if [[ "$LOGGED" == "Y" ]]; then
    echo "Building" >> $LOG
    echo "============================================================" >> $LOG
fi

# Build Berti
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
cd $DIR

#----------------------------------------------------------------------------#
#                                Running Simulations                         #
#----------------------------------------------------------------------------#
mkdir $OUT > /dev/null 2>&1

if [[ "$LOGGED" == "Y" ]]; then
    echo "RUNNING" >> $LOG
    echo "============================================================" >> $LOG
fi

if [[ "$PARALLEL" == "Y" ]]; then
    # Prepare to run in parallel
    echo -n "Making everything ready to run..."
    BINARY=$(ls $BERTI/bin)

    if [[ "$LOGGED" == "Y" ]]; then
        echo "$BINARY" >> $LOG
        strings -a $BERTI/bin/$BINARY | grep "GCC: " >> $LOG 2>&1
    fi

    file_trace $BERTI/bin/$BINARY $TRACES_SPEC $BINARY "Berti" > tmp_par.out

    for i in $(ls $PF/bin); do
        if [[ "$LOGGED" == "Y" ]]; then
            echo "$PF/bin/$i" >> $LOG
            strings -a $PF/bin/$i | grep "GCC: " >> $LOG 2>&1
        fi

        file_trace $PF/bin/$i $TRACES_SPEC $i $i >> tmp_par.out
    done
    echo " ${GREEN}done${NC}"

    # Run in parallel
    echo -n "Running..."
    cat tmp_par.out | xargs -I CMD -P $NUM_THREAD bash -c CMD
    echo " ${GREEN}done${NC}"
else
    # Running Berti
    BINARY=$(ls $BERTI/bin)

    if [[ "$LOGGED" == "Y" ]]; then
        echo "$BINARY" >> $LOG
        strings -a $BINARY | grep "GCC: " >> $LOG 2>&1
    fi

    run_trace $BERTI/bin/$BINARY $TRACES_SPEC $BINARY "Berti"
    # Runnin other PF
    for i in $(ls $PF/bin); do
        name=$(echo $i | cut -d'-' -f3)
        if [[ "$name" == "ipcp_isca2020" ]]; then
            name="IPCP"
        elif [[ "$name" == "ip_stride" ]]; then
            name="IP Stride"
        elif [[ "$name" == "mlop_dpc3" ]]; then
            name="MLOP"
        fi

        if [[ "$LOGGED" == "Y" ]]; then
            echo "$PF/bin/$i" >> $LOG
            strings -a $PF/bin/$i | grep "GCC: " >> $LOG 2>&1
        fi


        run_trace $PF/bin/$i $TRACES_SPEC $i $name
    done
fi

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
echo -n "Parsing data..."
if [[ "$VERBOSE" == "Y" ]]; then
    python3 Python/get_data.py y output > single.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data.py y output > single.csv 2>>$LOG
else
    python3 Python/get_data.py y output > single.csv 2>/dev/null
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
    python3 Python/get_data_fig.py y output >> single.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data_fig.py y output >> single.csv 2>>$LOG
else
    python3 Python/get_data_fig.py y output >> single.csv 2>/dev/null
fi
run_command "python3 Python/one_prefetch_performance.py single.csv"
echo -n "Generating Figure 9 (a)..."
if [[ "$VERBOSE" == "Y" ]]; then
    python3 Python/get_data_by_traces.py y SpeedUp output > spec.csv
elif [[ "$LOGGED" == "Y" ]]; then
    python3 Python/get_data_by_traces.py y SpeedUp output > spec.csv 2>>$LOG
else
    python3 Python/get_data_by_traces.py y SpeedUp output > spec.csv 2>/dev/null
fi
run_command "python3 Python/by_app_performance.py spec.csv cpu"
echo -n "Generating Figure 10 SPEC17-MemInt..."
run_command "python3 Python/l1d_accuracy.py single.csv"

 echo -n "Removing Temporal Files..."
 run_command "rm single.csv spec.csv tmp_par.out"

 if [[ "$REMOVE_ALL" == "Y" ]]; then
     echo -n "Removing All Files..."
     run_command "rm -rf output traces gcc7.5 ChampSim/Berti/bin ChampSim/Other_PF/bin"
 fi
