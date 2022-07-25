traces=$(cat scripts/spec2017_trace_list.txt)

for trace in $traces
do
	trace_name=${trace::-17}
	echo $trace_name
done
