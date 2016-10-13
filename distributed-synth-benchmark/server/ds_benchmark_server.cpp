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
#include <math.h>
#include <map>

/** Port to listen on */
#define SERVER_PORT         56636

/** Book keeping interval in seconds */
#define BOOK_KEEP_INTERVAL  30ULL

/** Internal value, easier to use */
#define BOOK_KEEP_INTERVAL_NS       BOOK_KEEP_INTERVAL * 1000000000ULL

/* Clock ID to use */
clockid_t ClockSource = CLOCK_MONOTONIC_RAW;

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

/** Message format; needs to stay in sync with the client */
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


struct StabilityParams
{
    double NumObservations;
    double Mean, Mean2;
    double Min, Max;
};

/** Updates state of an online std dev calculation. */
void UpdateObservation(struct StabilityParams* Params, double Value)
{
    double Delta;

    ++Params->NumObservations;

    Delta = Value - Params->Mean;

    Params->Mean += Delta / Params->NumObservations;
    Params->Mean2 += Delta * (Value - Params->Mean);

    if (Params->NumObservations > 1)
    {
        Params->Min = (Value < Params->Min) ? Value : Params->Min;
        Params->Max = (Value > Params->Max) ? Value : Params->Max;
    }
    else
    {
        Params->Min = Value;
        Params->Max = Value;
    }
}

/** Calculates values and prints them. */
void PrintValues(struct StabilityParams* Params)
{
    double Variance = 0, StandardDeviation = 0, RelativeStdDev = 0;

    if (Params->NumObservations > 1)
    {
        Variance = Params->Mean2 / (Params->NumObservations - 1);
        StandardDeviation = sqrt( Variance );

        if (Params->Mean * Params->Mean > 0.0001)
        {
                RelativeStdDev = 100.0 * StandardDeviation / Params->Mean;
        }
    }

    printf("Min(ms), %.1f, Max(ms), %.1f, Mean(ms), %.1f, StdDev(ms), %.1f, RelStdDev(%%), %.1f, DataSize, %.f", 
        Params->Min, Params->Max, Params->Mean, StandardDeviation, RelativeStdDev, Params->NumObservations);
}

struct Client
{
    /** Unique Id */
    unsigned long long      UniqueId;

    /** Time in nanoseconds we last time heard from them. */
    unsigned long long      LastTimeHeard;
};

std::map<unsigned long long, Client> Clients;

StabilityParams PacketTimes_AllTime;
StabilityParams PacketTimes_SinceLastBookkeep;
StabilityParams FrameTimes_AllTime;
StabilityParams FrameTimes_SinceLastBookkeep;
size_t AllTimeClients = 0;

void UpdateClient(const Message& Msg)
{
    unsigned long long Timestamp = GetTimeInNs();
    std::map<unsigned long long, Client>::iterator ClientIter = Clients.find(Msg.UniqueId);

    if (ClientIter == Clients.end())
    {
        // new client
        Client New;
        New.UniqueId = Msg.UniqueId;
        New.LastTimeHeard = Timestamp;

        Clients.insert(std::map<unsigned long long, Client>::value_type(Msg.UniqueId, New));
    }
    else
    {
        unsigned long long Delta = Timestamp - ClientIter->second.LastTimeHeard;
        ClientIter->second.LastTimeHeard = Timestamp;

        double DeltaMs = (double)(Delta) / 1000000.0;
        UpdateObservation(&PacketTimes_AllTime, DeltaMs);
        UpdateObservation(&PacketTimes_SinceLastBookkeep, DeltaMs);
    }

    double FrameTimeMs = (double)(Msg.FrameTimeNs) / 1000000.0;
    UpdateObservation(&FrameTimes_AllTime, FrameTimeMs);
    UpdateObservation(&FrameTimes_SinceLastBookkeep, FrameTimeMs);
}

/** Prints stats and removes clients we haven't heard from in a while. */
void DoBookkeeping(unsigned long long CurrentTime)
{
    AllTimeClients = std::max(AllTimeClients, Clients.size());
    printf("AllTime, Clients, %Zu, PacketTimes, ", AllTimeClients);
    PrintValues(&PacketTimes_AllTime);
    printf(" FrameTimes, ");
    PrintValues(&FrameTimes_AllTime);
    printf("   Current, Clients, %Zu, PacketTimes, ", Clients.size());
    PrintValues(&PacketTimes_SinceLastBookkeep);
    printf(" FrameTimes, ");
    PrintValues(&FrameTimes_SinceLastBookkeep);

    time_t Time = time(nullptr);
    struct tm* UtcTime = gmtime(&Time);
    printf(", %s", asctime(UtcTime));

    // reset
    memset(&PacketTimes_SinceLastBookkeep, 0, sizeof(PacketTimes_SinceLastBookkeep));
    memset(&FrameTimes_SinceLastBookkeep, 0, sizeof(FrameTimes_SinceLastBookkeep));

    // remove all clients we haven't heard from during this period
    for(std::map<unsigned long long, Client>::iterator It = Clients.begin(); It != Clients.end();)
    {
        if (CurrentTime - It->second.LastTimeHeard >= BOOK_KEEP_INTERVAL_NS)
        {
            Clients.erase(It++);
        }
        else
        {
            ++It;
        }
    }
}

int main(int argc, const char* argv[])
{
    setlinebuf(stdout);
    printf("Distributed synth benchmark server.\n");

    // non-blocking, because we want to book keep clients between receptions
    int Socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (Socket < 0) 
    {
        perror("Cannot create UDP socket");
        return 1;
    }

    int Opt = 1;
    // this is so port can be reused quickly, useful while testing
    setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&Opt, sizeof(Opt));

    struct sockaddr_in MyAddr;
    memset(&MyAddr, 0, sizeof(MyAddr));
    MyAddr.sin_family = AF_INET;
    MyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    MyAddr.sin_port = htons(SERVER_PORT);
    if (bind(Socket, (struct sockaddr *)&MyAddr, sizeof(MyAddr)) < 0)
    {
        close(Socket);
        return 1;
    }

    printf("Listening on port %d.\n", SERVER_PORT);
    printf("Stats printed each %llu seconds.\n", BOOK_KEEP_INTERVAL);

    /* Enter infinite loop - server never sleeps for better measurements */
    unsigned long long LastBookkeep = GetTimeInNs();
    for (;;)
    {
        // we only expect to receive unique ids
        Message IncomingMsg;
        memset(&IncomingMsg, 0, sizeof(IncomingMsg));
        int Len = recvfrom(Socket, &IncomingMsg, sizeof(IncomingMsg), 0, nullptr, nullptr);
        if (Len == -1)
        {
            if (errno != EAGAIN)
            {
                fprintf(stderr, "recvfrom() failed with errno = %d (%s).\n", errno, strerror(errno));
                close(Socket);
                return 1;
            }
        }
        else if (Len == sizeof(IncomingMsg))
        {
            UpdateClient(IncomingMsg);
        }
	else
	{
		printf("Received malformed message of %d bytes\n", Len);
	}

        unsigned long long CurrentTime = GetTimeInNs();
        if (CurrentTime - LastBookkeep > BOOK_KEEP_INTERVAL_NS)
        {
            DoBookkeeping(CurrentTime);
            LastBookkeep = CurrentTime;
        }
    }

    /** Never reached, but just in case. */
    close(Socket);
    return 0;
}
