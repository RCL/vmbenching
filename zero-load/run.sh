#!/bin/bash
# Assumes at least 16 core machine, uses core 0 and 4

trap CtrlCHandler INT
trap CtrlCHandler TERM
CtrlCHandler() 
{
	killall zero_load > /dev/null 2>&1
	exit 0
}

RunForever()
{
	./zero_load &
	TESTPID=$!

	taskset -c -p 0 $TESTPID

	while true; do
		kill -0 $TESTPID > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID exited"
			exit 0
		fi

		sleep 5
	done
}

taskset -c -p 4 $$

RunForever

