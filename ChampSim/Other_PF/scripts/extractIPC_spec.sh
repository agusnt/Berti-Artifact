#numoftraces=`wc cloudsuite_trace_list.txt | awk '{print $1}'`

if [ "$#" -lt 7 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./extractIPC_spec.sh [L1I PREF] [L1D PREF] [L2C PREF] [LLC PREF] [ITLB PREF] [DLTB PREF] [STLB PREF]"
    exit 1
fi

traces=$(cat scripts/intensive_trace_list.txt)
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
numsim=100

#results_dir="results_"$numsim"M/"
results_dir="results_100M"
baseline_dir="results_saved/SPEC2017/"

#calc(){ awk "BEGIN { print "$*" }"; }

#1. IPC
for trace in $traces
do 
	filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
	ipc=`awk '/CPU 0 cumulative IPC/ {print $5}' ${results_dir}/${filename}`
	echo $ipc
done

echo " " 
echo " "
echo " "
echo " " 

#2. L1D Pref Coverage

if [ $l1dpref == "no" ]
then
	#blanks....

	i=0

	while [ $i -lt 50 ]
	do
	       echo " "
	       i=`expr $i + 1`
	done

else


	for trace in $traces
	do
		filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
		baseline_file=${trace}-$branchPredictor-$l1ipref-no-no-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
		L1D_pref_misses=`awk '/L1D LOAD      ACCESS:/ {print $8}' ${results_dir}/${filename}`
		baseline_misses=`awk '/L1D LOAD      ACCESS:/ {print $8}' ${baseline_dir}/${baseline_file}`
		misses_covered=`expr $baseline_misses - $L1D_pref_misses`

		coverage=`echo "$misses_covered / $baseline_misses" | bc -l`
		coverage=`echo "if($coverage<1) print 0; $coverage" | bc`
		coverage=`echo "$coverage * 100" | bc -l`
		printf "%.6f\n" "$(bc -l <<< $coverage)"
		#echo $coverage
	done

fi 
#blanks....

#i=0

#while [ $i -lt 50 ]
#do
#	echo " "
#	i=`expr $i + 1`
#done

echo " "
echo " "
echo " "
echo " "

#3. L1D Pref Accuracy

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        accuracy=`awk '/L1D USEFUL LOAD PREFETCHES:/ {print $13}' $results_dir/${filename}`
        echo $accuracy
done

echo " " 
echo " "
echo " "
echo " "


#4. L1D Pref Lateness

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        timely_pref=`awk '/L1D TIMELY PREFETCHES:/ {print $4; exit;}' ${results_dir}/${filename}`
        late_pref=`awk '/L1D TIMELY PREFETCHES:/ {print $7; exit;}' ${results_dir}/${filename}`
        total=`expr $timely_pref + $late_pref`
	
	#echo "Timely pref $timely_pref"
	#echo "Late pref: $late_pref"
	#echo "Total: $total"

        lateness=`echo "$late_pref / $total" | bc -l`
        lateness=`echo "if($lateness<1) print 0; $lateness" | bc`
        lateness=`echo "$lateness * 100" | bc -l`
        printf "%.6f\n" "$(bc -l <<< $lateness)"
        #echo $lateness
done
echo " "
echo " "
echo " "
echo " "

#5. L1D MPKI 

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        l1dmpki=`awk '/L1D LOAD      ACCESS:/ {print $16}' $results_dir/${filename}`
        echo $l1dmpki
done

echo " "
echo " "
echo " "
echo " "


#6. DTLB MPKI

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        dtlbmpki=`awk '/DTLB LOAD TRANSLATION ACCESS:/ {print $17}' $results_dir/${filename}`
        echo $dtlbmpki
done

echo " "
echo " "
echo " "
echo " "


#7. L1-I MPKI

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        l1impki=`awk '/L1I LOAD      ACCESS:/ {print $16}' $results_dir/${filename}`
        echo $l1impki
done

echo " "
echo " "
echo " "
echo " "


#8. ITLB MPKI

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        itlbmpki=`awk '/ITLB LOAD TRANSLATION ACCESS:/ {print $17}' $results_dir/${filename}`
        echo $itlbmpki
done

echo " "
echo " "
echo " "
echo " "


#9. STLB MPKI

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        stlbmpki=`awk '/STLB LOAD TRANSLATION ACCESS:/ {print $17}' $results_dir/${filename}`
        echo $stlbmpki
done

echo " "
echo " "
echo " "
echo " "


#10. L2C MPKI 

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        l2cmpki=`awk '/L2C LOAD      ACCESS:/ {print $16}' $results_dir/${filename}`
        echo $l2cmpki
done

echo " "
echo " "
echo " "
echo " "


#11. LLC MPKI

for trace in $traces
do
        filename=${trace}-$branchPredictor-$l1ipref-$l1dpref-$l2cpref-$llcpref-$itlbpref-$dtlbpref-$stlbpref-$repl-1core.txt
        llcmpki=`awk '/LLC LOAD      ACCESS:/ {print $16}' $results_dir/${filename}`
        echo $llcmpki
done


#CPU 0 Average Load ROB stall cycles: 2.2918
#CPU 0 Fraction of times ROB stalled by Loads: 16.7929
#CPU 0 Percentage of Load stall cycles in Total stall cycles: 25.951
#CPU 0 NUM of IPS identified critical: 3

