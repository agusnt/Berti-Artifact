#log2_region_size=14

#while [ $log2_region_size -lt 21 ]
#do 
#	degree=3

#	while [ $degree -lt 9 ]
#	do

#		a=$(ls ../results_1M | grep "ip_stride_region-"$log2_region_size"-"$degree"-no-no")
#		i=0
#		for x in $a
#		do
#			i=`expr $i + 1`
#			echo $i
#			cat "../results_1M/$x" | grep "CPU 0 cumulative IPC"  | awk 'NR==1' | awk -F ' ' '{print $5}' >> "../Statistics/IP_stride_region_"$log2_region_size"_"$degree".txt"

#		done

#		degree=`expr $degree + 1`
#	done
#	log2_region_size=`expr $log2_region_size + 1`
#done

#num_ips=2
dist=0

Predictor=hashed_perceptron

#l1ipref=CURRENTLY SETTING STATIC.
l1dpref=(no) #bingo_64KB bingo_48KB bingo_34KB bingo_18KB bingo_11KB bingo_8KB bingo_5KB final_ipcp_1024entr final_ipcp_512entr final_ipcp_256entr final_ipcp_128entr final_ipcp_64entr) #idealL1Dpref no final_ipcp final_ipcp final_ipcp no no no ipcp_crossPages ipcp_crossPages bingo_dpc3)
l2cpref=(no) # no no no no no no no no no no no) # no no ipcp_practical spp_tuned_aggr spp_tuned_aggr spp ppf no spp_tuned_aggr no)
llcpref=(no) # no no no no no no no no no no no)

#tlb prefetchers
itlb_pref=(no) # no no no no no no no no no no no)
dtlb_pref=(no) # no no no no no no no no no no no)
stlb_pref=(no) # no no no no no no no no no no no)


llcrepl=srrip-drrip

numofpref=${#l1dpref[@]}


#while [ $dist -lt 1 ]
for((i=0; i<$numofpref; i++))
       do

               #a=$(ls ../results_5M | grep "ipcp_crossPages_distance-dist"$dist"-no-no-no-no-no")
	       a=$(ls ../results_5M | grep "mana_32KB-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-${itlb_pref[i]}-${dtlb_pref[i]}-${stlb_pref[i]}")
	      # echo $a

	       b=$(ls ../results_10M | grep "6*-no-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-${itlb_pref[i]}-${dtlb_pref[i]}-${stlb_pref[i]}")
	       #echo $b
	       #exit
	       echo "For IPC client-server:"
               j=0
               for x in $a
               do
                       j=`expr $j + 1`
                       echo $j
                       cat "../results_5M/$x" | grep "CPU 0 cumulative IPC"  | awk 'NR==1' | awk -F ' ' '{print $5}' >> "../Statistics/IPC_traces/5M-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-${itlb_pref[i]}-${dtlb_pref[i]}-${stlb_pref[i]}.txt"

               done

	       echo "For SPEC2017:"
	       j=0
	       for x in $b
	       do
		       j=`expr $j + 1`
		       echo $j
		       cat "../results_10M/$x" | grep "CPU 0 cumulative IPC"  | awk 'NR==1' | awk -F ' ' '{print $5}' >> "../Statistics/SPEC2017/10M-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-${itlb_pref[i]}-${dtlb_pref[i]}-${stlb_pref[i]}.txt"

	       done

               #dist=`expr $dist + 1`
       done

