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
#include <sys/socket.h>
#include <arpa/inet.h>

/** Server frame rate, Hz. We are trying to maintain it. */
#define SERVER_FPS          30ULL

/** Budget for a single frame in nanoseconds, given target FPS */
#define SERVER_FRAME_DURATION_NS        (1000000000ULL / SERVER_FPS)

/** Size of the working set - arbitrary, timed to make sure we can keep up given Hz */
#define WORKSET_SIZE_SQRT   256UL

/** Total size of a workset element - allows to spread out it across the memory */
#define WORKSET_ELEMENT_SIZE	256UL


/* Clock ID to use */
clockid_t ClockSource = CLOCK_MONOTONIC_RAW;

/* Memory that is used to imitate useful work */
char* WorkSet = NULL;

/** Gets current time in ns */
unsigned long long GetTimeInNs()
{
    struct timespec TimeSpec;
    
    if (clock_gettime(ClockSource, &TimeSpec) == -1)
    {
        fprintf(stderr, "Cannot use clock source %d\n", ClockSource);
        exit(1);
        return 0;
    };

    return (unsigned long long)(TimeSpec.tv_sec) * 1000000000ULL + (unsigned long long)(TimeSpec.tv_nsec);
}

#define TUNING 0

/** Spends "working", fixed cost. Nanoseconds budget is only used when tuning the work for a specific machine. */
void SpendTimeWorking(unsigned long long Nanoseconds)
{
#if TUNING
    unsigned long long StartNs = GetTimeInNs(), CurrentNs, DiffNs;
    int TimesRun = 0;
#endif // TUNING
    int WorkSetElementIdx = 0;

    for(;;)
    {
#if TUNING
        CurrentNs = GetTimeInNs();

        DiffNs = CurrentNs - StartNs;
        if (DiffNs >= Nanoseconds)
        {
            printf("TimesRun = %d\n", TimesRun);
            break;
        }
#endif // TUNING

        /* Imitate useful work, memory bound. Low count initially since it grows non-linearly with the machines */
#if !TUNING
        for (int Count = 0; Count < 2; ++Count)
#else
	++TimesRun;
#endif
        {
            for (int IdxColumn = 0; IdxColumn < WORKSET_SIZE_SQRT; ++IdxColumn)
            {
                for (int IdxRow = 0; IdxRow < WORKSET_SIZE_SQRT; ++IdxRow)
                {
                    WorkSet[(IdxRow * WORKSET_SIZE_SQRT + IdxColumn) * WORKSET_ELEMENT_SIZE + WorkSetElementIdx] 
                        = WorkSet[(IdxColumn * WORKSET_SIZE_SQRT + IdxRow) * WORKSET_ELEMENT_SIZE + WorkSetElementIdx];
                    WorkSetElementIdx = (WorkSetElementIdx + 1) % WORKSET_ELEMENT_SIZE;
                }
            }
        }
#if !TUNING
        break;
#endif // !TUNING
    }
}

/** Sleep no less than TimeToSleepNs nanoseconds, properly accounting for signals */
void Sleep(unsigned long long TimeToSleepNs)
{
    struct timespec TimeSpec, TimeSpecRemain;
    int SleepResult;

    TimeSpec.tv_sec = 0;
    TimeSpec.tv_nsec = TimeToSleepNs;
    for (;;)
    {
        /* Cannot sleep on CLOCK_MONOTONIC_RAW */
        SleepResult = clock_nanosleep(CLOCK_MONOTONIC, 0, &TimeSpec, &TimeSpecRemain);
        if (SleepResult == 0)
        {
            break;
        }
        else if (SleepResult != EINTR)
        {
            fprintf(stderr, "clock_nanosleep() failed: SleepResult = %d\n", SleepResult);
            exit(1);
        }

        // got interrupted, repeat
        memcpy(&TimeSpec, &TimeSpecRemain, sizeof(TimeSpec));
    }
}

/** Message format; needs to stay in sync with the server */
#pragma pack(push, 1)
struct Message
{
	/** Unique Id of this client */
	unsigned long long UniqueId;
	/** Actual frame time in nanoseconds */
	unsigned long long FrameTimeNs;
	/** Number of the frame (and message, because it is sent once per frame). */
	unsigned long long FrameNumber;
};
#pragma pack(pop)

int main(int argc, const char* argv[])
{
    struct sockaddr_in ServerAddr;
    int Socket, Port = 56636;
    const char* ServerURL = "127.0.0.1";
    unsigned long long BeginFrameNs, UsefulWorkTimeNs, ActualFrameTimeNs;
    unsigned long long UniqueId;
    struct Message Msg;
    FILE* DevUrandom = NULL;

    if (argc >= 2) 
    {
        ServerURL = argv[1];
    }

    if (argc >= 3) 
    {
        Port = atoi(argv[2]);
    }

    printf("Distributed synth benchmark client.\n");
    printf("Reporting to %s:%d (use %s [server] [port] to override)\n", ServerURL, Port, argv[0]);

    Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (Socket < 0) 
    {
        perror("Cannot create UDP socket");
        return 1;
    }

    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    if (inet_pton(AF_INET, ServerURL, &ServerAddr.sin_addr) == 0) 
    {
        perror("Cannot convert server address to binary, make sure it is given as an IPv4 and not a hostname.");
        close(Socket);
        return 1;
    }

    /* Read our own unique id */
    DevUrandom = fopen("/dev/urandom", "rb");
    if (DevUrandom == NULL)
    {
        perror("Cannot open /dev/urandom to read unique id");
        close(Socket);
        return 1;
    }

    if (fread(&UniqueId, sizeof(UniqueId), 1, DevUrandom) != 1)
    {
        perror("Cannot read from /dev/urandom");
        close(Socket);
        fclose(DevUrandom);
        return 1;
    }
    fclose(DevUrandom);
    
    printf("This client unique id is 0x%llx\n", UniqueId);

    WorkSet = malloc(WORKSET_SIZE_SQRT * WORKSET_SIZE_SQRT * WORKSET_ELEMENT_SIZE);
    if (WorkSet == NULL)
    {
        perror("Cannot allocate memory for working set");
        return 1;
    }

    memset(&Msg, 0, sizeof(Msg));
    Msg.UniqueId = UniqueId;
    Msg.FrameNumber = 0;    
    BeginFrameNs = GetTimeInNs();
    /* Enter infinite loop */
    for (;;)
    {
        SpendTimeWorking(SERVER_FRAME_DURATION_NS / 2ULL);

        /* Sleep for the rest of the frame */
        UsefulWorkTimeNs = GetTimeInNs() - BeginFrameNs;
        if (UsefulWorkTimeNs < SERVER_FRAME_DURATION_NS)
        {
            Sleep(SERVER_FRAME_DURATION_NS - UsefulWorkTimeNs);
        }

        ActualFrameTimeNs = GetTimeInNs() - BeginFrameNs;
	Msg.FrameTimeNs = ActualFrameTimeNs;

        /* debug only - used to time the working set
        printf("Frame time is %llu ns, non-sleep time is %llu ns\n", ActualFrameTimeNs, UsefulWorkTimeNs);
        */
        
        if (sendto(Socket, &Msg, sizeof(Msg), 0, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) == -1)
        {
            perror("sendto failed");
            free(WorkSet);
            close(Socket);
            return 1;
        }

        ++Msg.FrameNumber;
        BeginFrameNs = GetTimeInNs();
    }

    /** Never reached, but just in case. */
    free(WorkSet);
    close(Socket);

    return 0;
};

