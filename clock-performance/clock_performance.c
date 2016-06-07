/*

Copyright (c) 2016 Epic Games, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void *ThreadFunc(void *Data) 
{
	unsigned long long IdxIter, NumIterations = *(unsigned long long *)Data;
	struct timespec TimeSpec;

	for (IdxIter = 0; IdxIter < NumIterations; ++IdxIter)
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
	}
}

int main(int argc, char **argv) 
{
	pthread_t* Threads;
	int NumThreads = 1, IdxThread;
	unsigned long long NumIterations = 1000000ULL;

	if (argc > 1)
	{
		NumIterations = atol(argv[1]);
	}

	if (argc > 2)
	{
		NumThreads = atoi(argv[2]);
	}

	Threads = (pthread_t *)malloc(NumThreads * sizeof(pthread_t));

	for (IdxThread = 0; IdxThread < NumThreads; ++IdxThread)
	{
		pthread_create(Threads + IdxThread, NULL, ThreadFunc, &NumIterations);
	}

	for (IdxThread = 0; IdxThread < NumThreads; ++IdxThread)
	{
		pthread_join(Threads[IdxThread], NULL);
	}

	free(Threads);
	Threads = NULL;

	return 0;
}



