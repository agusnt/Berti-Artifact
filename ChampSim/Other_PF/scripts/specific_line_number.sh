traces=$(cat scripts/intensive_trace_list.txt)

for trace in $traces;
do
	awk '/TIMELY PREFETCHES/ && NR==97{print $6}' results_saved/Prefetch/$trace-hashed_perceptron-ipcp_crossPages-spp_tuned_aggr-no-no-no-no-lru-1core.txt
done
