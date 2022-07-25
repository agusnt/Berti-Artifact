#numoftraces=`wc cloudsuite_trace_list.txt | awk '{print $1}'`

if [ "$#" -lt 6 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./extractIPC_cloudsuite.sh [L1D PREF] [L2C PREF] [LLC PREF] [ITLB PREF] [DTLB PREF] [STLB PREF]"
    exit 1
fi

traces=$(cat scripts/cloudsuite_trace_list.txt)
branchPredictor=hashed_perceptron
l1dpref=${1}
l2cpref=${2}
llcpref=${3}
itlbpref=${4}
dtlbpref=${5}
stlbpref=${6}
llcrepl=srrip-drrip
numcore=1
numsim=50

cur_trace="TEMP"
cur_ipc=0

for trace in $traces
do 
	filename=${trace}-$branchPredictor-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$llcrepl-1core-cloudsuite.txt
        ipc=`awk '/CPU 0 cumulative IPC/ {print $5}' results_${numsim}M/cloudsuite/${filename}`
	#echo $ipc
	temp_trace=${trace::-10}
	if [ $temp_trace == $cur_trace ]
	then
		cur_ipc=`echo $cur_ipc + $ipc | bc -l`
	else
		if [ "$cur_ipc" != "0" ]
		then
		#temp=${temp_trace::-5}
		#echo $temp
			echo "if($cur_ipc<1) print 0; $cur_ipc" | bc
		#	echo $cur_ipc
		fi
		cur_ipc=$ipc
		cur_trace=$temp_trace
	fi
done

echo "if($cur_ipc<1) print 0; $cur_ipc" | bc
