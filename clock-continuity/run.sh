#!/bin/bash
# Assumes at least 16 core machine, uses core 8 and 12

trap CtrlCHandler INT
trap CtrlCHandler TERM
CtrlCHandler() 
{
	killall clock_continuity > /dev/null 2>&1
	exit 0
}

RunForever()
{
	./clock_continuity &
	TESTPID=$!

	taskset -c -p 8 $TESTPID

	while true; do
		kill -0 $TESTPID > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID exited"
			exit 0
		fi

		sleep 5
	done
}

taskset -c -p 12 $$

RunForever

