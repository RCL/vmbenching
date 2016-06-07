#!/bin/bash
# Assumes at least 8 core machine, uses cores 0,5

trap CtrlCHandler INT
trap CtrlCHandler TERM
CtrlCHandler() 
{
	killall clock_stability > /dev/null 2>&1
	exit 0
}

RunForever()
{
	./clock_stability 30 | tee stability.log &
	TESTPID=$!
	taskset -c -p 5 $TESTPID

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

gcc -O2 clock_stability.c -lm -lrt -o clock_stability
if [ $? -ne 0 ]; then
	exit 1
fi

RunForever

