#!/bin/bash
#
# Copyright (c) 2016 Epic Games, Inc.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# captures ctrl-c during testing
trap early_exit_graceful INT
trap early_exit_graceful TERM
early_exit_graceful()
{
	killall ds_benchmark_client > /dev/null 2>&1
	exit 0
}

if [ $# -lt 1 ]; then
	echo "Usage: ./run_multiple <num_instances> <server_ip>"
	exit 1
fi

num=$1
server_ip=$2

run_test1() {
	local pids

	for (( i=1; i <= $num; i++ ))
	{
		./ds_benchmark_client $server_ip &
		pids[$i]=$!
		#taskset -p -c $((i-1)) ${pids[$i]}
		echo "watching pid ${pids[$i]} - instance #$i (pinned to core $((i-1)))"
		#sleep 5
	}

	while true; do
		local kill_all=0
		for (( i=1; i <= $num; i++ ))
		{
			kill -0 ${pids[$i]} > /dev/null 2>&1
			if [ $? -ne 0 ]; then
				echo "pid ${pids[$i]} exited"
				kill_all=1
			fi
		}

		if [ $kill_all -ne 0 ]; then
			for (( i=1; i <= $num; i++ ))
			{
				kill ${pids[$i]} > /dev/null 2>&1
			}

			exit 0;
		fi

		sleep 5
	done
}

run_test1

