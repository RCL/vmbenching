#!/bin/sh

gcc -O2 clock_performance.c -lrt -lpthread -o clock_performance
if [ $? -ne 0 ]; then
	exit 1
fi

printf "NumCores,\tNumIter,\tRealTime (sec),\tSystemTime (sec),\tUserTime (sec)\n"

NumIter=10000000
MaxCores=$(getconf _NPROCESSORS_ONLN)

for NumCores in $(seq 1 $MaxCores); do
	TimeResults="$(/usr/bin/time --format '%e %S %U' ./clock_performance $NumIter $NumCores 2>&1 1>/dev/null )"
	printf "%d,\t" $NumCores $NumIter
	printf "%s,\t" $TimeResults
	printf "\n"
done


