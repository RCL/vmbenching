#!/bin/bash
# Assumes at least 8 core machine, uses cores 0,2,4,6

trap CtrlCHandler INT
trap CtrlCHandler TERM
CtrlCHandler() 
{
	killall clock_continuity > /dev/null 2>&1
	exit 0
}

RunForever()
{
	./clock_continuity 100 &
	TESTPID100=$!
	taskset -c -p 2 $TESTPID100

	./clock_continuity 5 &
	TESTPID5=$!
	taskset -c -p 4 $TESTPID5

	./clock_continuity 1 &
	TESTPID1=$!
	taskset -c -p 6 $TESTPID1

	while true; do
		kill -0 $TESTPID100 > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID100 exited"
			exit 0
		fi

		kill -0 $TESTPID5 > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID5 exited"
			exit 0
		fi

		kill -0 $TESTPID1 > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "pid %TESTPID1 exited"
			exit 0
		fi


		sleep 5
	done
}

taskset -c -p 0 $$

RunForever

