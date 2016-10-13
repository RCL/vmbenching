#!/bin/bash
# Assumes at least 8 core machine, uses cores 0,5

trap CtrlCHandler INT
trap CtrlCHandler TERM
CtrlCHandler() 
{
	killall ds_benchmark_server > /dev/null 2>&1
	exit 0
}

RunForever()
{
	./ds_benchmark_server > >(tee ds_benchmark_server.log) &
	TESTPID=$!
	taskset -c -p 2 $TESTPID

	while true; do
		kill -0 $TESTPID > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID exited"
			exit 0
		fi

		sleep 5
	done
}

taskset -c -p 0 $$

RunForever

