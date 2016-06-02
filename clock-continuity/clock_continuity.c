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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, const char* argv[])
{
	struct timespec TimeSpec;
	unsigned long long ResolutionNs = 0, PrevNs = 0, CurrentNs = 0;
	unsigned long long DiffNs = 0;
	unsigned long long ThresholdNs = 100000000;	// 100 ms
	time_t Time;
	struct tm* UtcTime;
	int Cooldown = 2;	/* skip first two readings */

	/* Read threshold in millseconds from commandline, if any */
	if (argc > 1)
	{
		ThresholdNs = atol(argv[1]) * 1000000;
	}

	clock_getres(CLOCK_MONOTONIC_RAW, &TimeSpec);
	ResolutionNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

	printf("%d: Resolution of CLOCK_MONOTONIC_RAW is %llu nsec\n", getpid(), ResolutionNs);

	printf("%d: Largest tolerable difference between clock readings is %llu nsec (%llu ms)\n", getpid(), ThresholdNs, ThresholdNs / 1000000);

	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
	PrevNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

	printf("Checking if we ever see too large difference between clock readings (program never exits)\n");

	for (;;)
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
		CurrentNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

		DiffNs = CurrentNs - PrevNs;

		if (Cooldown == 0)
		{
			/* Check if we're ever too far off (larger than threshold) */
			if (DiffNs > ThresholdNs)
			{
				Time = time(NULL);
				UtcTime = gmtime(&Time);
			
				printf("pid %d: too large difference between clock readings, %llu nsec (largest tolerable difference is %llu nsec) at %s",
					getpid(),
					DiffNs, ThresholdNs,
					asctime(UtcTime)
				);

				Cooldown = 2;	/* skip next two readings because printing off the result might have taken too long. */
			}
		}
		else
		{
			--Cooldown;
		}

		PrevNs = CurrentNs;
	}

	return 0;
};

