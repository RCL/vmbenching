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
#include <math.h>

/** State for online mean and variance */
struct StabilityParams
{
	double NumObservations;
	double Mean, Mean2;
};

/** Updates values */
void UpdateObservation(struct StabilityParams* Params, double Value)
{
	double Delta;

	++Params->NumObservations;

	Delta = Value - Params->Mean;

	Params->Mean += Delta / Params->NumObservations;
	Params->Mean2 += Delta * (Value - Params->Mean);
}

/** Print values */
void PrintValues(struct StabilityParams* Params)
{
	double Variance = 0, StandardDeviation = 0;

	if (Params->NumObservations > 1)
	{
		Variance = Params->Mean2 / (Params->NumObservations - 1);
		StandardDeviation = sqrt( Variance );
	}

	printf("Mean: %.1f ns StandardDeviation: %.1f ns NumObservations: %.fM", Params->Mean, StandardDeviation, Params->NumObservations / 1000000);
}


int main(int argc, const char* argv[])
{
	struct timespec TimeSpec;
	unsigned long long ResolutionNs = 0, PrevNs = 0, CurrentNs = 0, LastPeriodStarted = 0;
	double DiffNs = 0;
	time_t Time;
	unsigned long long PeriodInSeconds = 600, PeriodInNs;
	struct tm* UtcTime;
	int Cooldown = 100;	/* skip first readings */
	struct StabilityParams AllTime, LastPeriod;

	if (argc > 1)
	{
		PeriodInSeconds = atol(argv[1]);
	}
	PeriodInNs = PeriodInSeconds * 1000000000ULL;


	memset(&AllTime, 0, sizeof(AllTime));
	memset(&LastPeriod, 0, sizeof(LastPeriod));

	clock_getres(CLOCK_MONOTONIC_RAW, &TimeSpec);
	ResolutionNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

	printf("%d: Resolution of CLOCK_MONOTONIC_RAW is %llu nsec\n", getpid(), ResolutionNs);
	printf("%d: Print interval in seconds is %llu\n", getpid(), PeriodInNs / 1000000000ULL);

	clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
	PrevNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);
	LastPeriodStarted = PrevNs;

	printf("Checking stability of the clock (program never exits)\n");

	for (;;)
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &TimeSpec);
		CurrentNs = (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);

		DiffNs = (double)(CurrentNs - PrevNs);

		if (Cooldown == 0)
		{
			UpdateObservation(&AllTime, DiffNs);
			UpdateObservation(&LastPeriod, DiffNs);

			/* Check if we're ever too far off (larger than threshold) */
			if (CurrentNs - LastPeriodStarted > PeriodInNs)
			{
				Time = time(NULL);
				UtcTime = gmtime(&Time);
			
				printf("pid %d: AllTime: ", getpid());
				PrintValues(&AllTime);
				printf("   Period: " );
				PrintValues(&LastPeriod);
				printf(" at %s", asctime(UtcTime));
				fflush(stdout);

				memset(&LastPeriod, 0, sizeof(LastPeriod));
				LastPeriodStarted = CurrentNs;

				Cooldown = 100;	/* skip next hundred readings because printing off the result might have taken too long. */
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

