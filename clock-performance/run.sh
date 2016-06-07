#!/bin/sh

gcc -O2 clock_performance.c -lrt -lpthread -o clock_performance
if [ $? -ne 0 ]; then
	exit 1
fi

echo "NumCores,\tNumIter,\tTotalTime (sec),\tSystemTime (sec),\tUserTime (sec)"

NumIter=10000000
MaxCores=$(getconf _NPROCESSORS_ONLN)

for NumCores in $(seq 1 $MaxCores); do
	TimeResults="$(/usr/bin/time --format '%e,\t%S,\t%U' ./clock_performance $NumIter $NumCores 2>&1 1>/dev/null )"
	echo "$NumCores,\t$NumIter,\t$TimeResults"
done


