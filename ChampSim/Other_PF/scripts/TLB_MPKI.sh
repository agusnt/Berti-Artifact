
total_workloads=`wc scripts/experiment_trace_list.txt | awk '{print $1}'`
policies=(lru)

#prefetchers=(next_line-next_line-next_line)
prefetchers=(no-constant_stride_4096-no)

results_folder_TLB=results_50M/experiment
output_filename=MPKI.csv

rm -f ${output_filename}
echo ",ITLB misses,DTLB misses, STLB misses" >> ${output_filename}
echo "trace_no,," >> ${output_filename}

#echo ", PerfectTLB" >> ${output_filename}
#echo "trace_no,IPC" >> ${output_filename}

count=0
for((i=1;i<=${total_workloads};i++));
do
	trace=`sed -n ''${i}'p' scripts/experiment_trace_list.txt | awk '{print $1}'`
	output=`echo ${trace}`	# | awk '{print substr($1,1,length($1)-17)}'`
	
	#filename=${trace}-bimodal-next_line-next_line-next_line-no-no-no-lru-1core.txt
	#no_cpi=`awk '/CPU 0 cumulative IPC/ {print $5}' ${results_folder_without_pref}/${filename}`
	#output=${output}','${no_cpi}


	count=`expr $count + 1`
	for TLBprefetcher in ${prefetchers[*]}
	do
		if [ $count -lt 12 ]
		then
			binary=bimodal-no-no-no-${TLBprefetcher}-lru-1core
		else
			binary=bimodal-no-no-no-${TLBprefetcher}-lru-1core-cloudsuite
		fi
		filename=${trace}-${binary}.txt
		##load=`awk '/ITLB LOAD/ {print $8}' ${results_folder_without_pref}/${filename}`
		##output=${output}','${load}
		sim_ins=`awk '/Finished/ {print $10}' ${results_folder_TLB}/${filename}`
		output=${output}','${sim_ins}
		load=`awk '/DTLB LOAD      ACCESS:/ {print $8}' ${results_folder_TLB}/${filename}`
        	#rfo=`awk '/DTLB RFO       ACCESS:/ {print $8}' ${results_folder_with_pref}/${filename}`
		#demand=`echo "${load}+${rfo}" | bc -l`
		output=${output}','${load}
		#output=${output}','${rfo}
		##load=`awk '/STLB LOAD      ACCESS:/ {print $8}' ${results_folder_without_pref}/${filename}`
		#rfo=`awk '/STLB RFO       ACCESS:/ {print $8}' ${results_folder_with_pref}/${filename}`
		#demand=`expr $load + $rfo`
		##output=${output}','${load}
		#output=${output}','${rfo}

	done
	#output=${output}','
	#for TLBprefetcher in ${prefetchers[*]}
        #do
        #        binary=bimodal-next_line-next_line-next_line-${TLBprefetcher}-lru-1core
        #        filename=${trace}-${binary}.txt
        #        get_cpi=`awk '/CPU 0 cumulative IPC/ {print $5}' ${results_folder_with_pref}/${filename}`
        #        improv=$(echo "scale=6; ${get_cpi}/${no_cpi}" | bc -l)
        #        output=${output}','${improv}
        #donei
	echo ${output} >> ${output_filename}
done
