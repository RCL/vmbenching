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



