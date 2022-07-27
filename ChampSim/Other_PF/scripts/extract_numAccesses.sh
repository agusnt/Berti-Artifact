#numoftraces=`wc cloudsuite_trace_list.txt | awk '{print $1}'`

if [ "$#" -lt 7 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./extractIPC_server.sh [L1I PREF] [L1D PREF] [L2C PREF] [LLC PREF] [ITLB PREF] [DLTB PREF] [STLB PREF]"
    exit 1
fi

traces=$(cat scripts/ipc_trace_list.txt)
#traces=$(cat scripts/intensive_trace_list.txt)
branchPredictor=hashed_perceptron
l1ipref=${1}
l1dpref=${2}
l2cpref=${3}
llcpref=${4}
itlbpref=${5}
dtlbpref=${6}
stlbpref=${7}
repl=lru-lru-lru-srrip-drrip-lru-lru-lru
numcore=1
numsim=50
#numsim=100

results_dir="results_"$numsim"M/"
#results_dir="results_saved/SPEC2017/"
#baseline_dir="results_saved/IPC_Traces/"

#1. IPC
for trace in $traces
do 
	filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
	data=`awk '/L1I TOTAL     ACCESS:/ {print $4}' ${results_dir}/${filename}`
	echo $data
done

echo " "
echo " "
echo " "
echo " "
echo " "


#1. IPC
for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        ipc=`awk '/L1D TOTAL     ACCESS:/ {print $4}' ${results_dir}/${filename}`
        echo $ipc
done

echo " "
echo " "
echo " "
echo " "
echo " "

#1. IPC
for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        ipc=`awk '/L2C TOTAL     ACCESS:/ {print $4}' ${results_dir}/${filename}`
        echo $ipc
done

echo " "
echo " "
echo " "
echo " "
echo " "

#1. IPC
for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        ipc=`awk '/LLC TOTAL     ACCESS:/ {print $4}' ${results_dir}/${filename}`
        echo $ipc
done

echo " "
echo " "
echo " "
echo " "
echo " "

#1. IPC
for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        rowbuf_hits=`awk '/RQ ROW_BUFFER_HIT:/ {print $3}' ${results_dir}/${filename}`
	rowbuf_misses=`awk '/RQ ROW_BUFFER_HIT:/ {print $5}' ${results_dir}/${filename}`
        total=`expr $rowbuf_hits + $rowbuf_misses`
	echo $total
done

echo " "
echo " "
echo " "
echo " "
echo " "

#1. IPC
for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        rowbuf_hits=`awk '/WQ ROW_BUFFER_HIT:/ {print $3}' ${results_dir}/${filename}`
        rowbuf_misses=`awk '/WQ ROW_BUFFER_HIT:/ {print $5}' ${results_dir}/${filename}`
        total=`expr $rowbuf_hits + $rowbuf_misses`
        echo $total
done

echo " "
echo " "
echo " "
echo " "
echo " "

