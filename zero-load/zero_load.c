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
	struct timespec TimeSpec, TimeSpecRemain;
	unsigned long long StartNs = 0, EndNs = 0, DiffNs = 0;
	time_t Time;
	struct tm* UtcTime;
	int SleepResult = 0;

	for (;;)
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
		StartNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

		TimeSpec.tv_sec = 0;
		TimeSpec.tv_nsec = 33000000;
		for (;;)
		{
			SleepResult = clock_nanosleep(CLOCK_MONOTONIC, 0, &TimeSpec, &TimeSpecRemain);
			if (SleepResult == 0)
			{
				break;
			}
			else if (SleepResult != EINTR)
			{
				Time = time(NULL);
				UtcTime = gmtime(&Time);
				
				printf("pid %d: clock_nanosleep() failed with %d (%s) at %s",
					getpid(),
					SleepResult,
					strerror(SleepResult),
					asctime(UtcTime)
				);

				exit(1);
			}

			// got interrupted, repeat
			memcpy(&TimeSpec, &TimeSpecRemain, sizeof(TimeSpec));
		}
	
		clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
		EndNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

		DiffNs = EndNs - StartNs;

		if (DiffNs > 100000000)	/* if instead of 33000 us it took more than 100000 us, something is really wrong */
		{
			Time = time(NULL);
			UtcTime = gmtime(&Time);

			printf( "pid %d: usleep(33000) took %llu microseconds instead of 33000, at %s",
				getpid(),
				(DiffNs + 500) / 1000,
				asctime(UtcTime)
			);
		}		
	}

	return 0;
};
