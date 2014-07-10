//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2014 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "bcm_host.h"

#include "cpuTrace.h"
#include "dynamicInfo.h"
#include "font.h"
#include "image.h"
#include "lcd.h"
#include "memoryTrace.h"
#include "staticInfo.h"
#include "syslogUtilities.h"

//-------------------------------------------------------------------------

volatile bool run = true;

//-------------------------------------------------------------------------

void
printUsage(
    const char *name)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s <options>\n", name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    --daemon - start in the background as a daemon\n");
    fprintf(stderr, "    --help - print usage and exit\n");
    fprintf(stderr, "\n");
}

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:

        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

int
main(
    int argc,
    char *argv[])
{
    char *program = basename(argv[0]);

    bool isDaemon =  false;

    //---------------------------------------------------------------------

    static const char *sopts = "dh";
    static struct option lopts[] = 
    {
        { "daemon", no_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'h' },
        { NULL, no_argument, NULL, 0 }
    };

    int opt = 0;

    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'd':

            isDaemon = true;
            break;

        case 'h':
        default:

            printUsage(program);

            if (opt == 'h')
            {
                exit(EXIT_SUCCESS);
            }
            else
            {
                exit(EXIT_FAILURE);
            }

            break;
        }
    }

    //---------------------------------------------------------------------

    if (isDaemon)
    {
        daemon(0, 0);
        openlog(program, LOG_PID, LOG_USER);
    }

    //---------------------------------------------------------------------

    if (access("/dev/mem", R_OK | W_OK) == -1)
    {
        perrorLog(isDaemon,
                  program,
                 "read and write access to /dev/mem required");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGINT signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGTERM signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    LCD_T lcd;

    if (initLcd(&lcd, 90) == false)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "LCD initialization failed");
        exit(EXIT_FAILURE);
    }

    clearLcd(&lcd, packRGB(0, 0, 0));

    //---------------------------------------------------------------------

    int16_t top = 0;

    STATIC_INFO_T staticInfo;
    top = initStaticInfo(lcd.width, top, &staticInfo);
    showStaticInfo(&lcd, &staticInfo);

    //---------------------------------------------------------------------

    DYNAMIC_INFO_T dynamicInfo;
    top = initDynamicInfo(lcd.width, top, &dynamicInfo);

    //---------------------------------------------------------------------

    CPU_TRACE_T cpuTrace;
    top = initCpuTrace(lcd.width, 80, top, &cpuTrace);

    //---------------------------------------------------------------------

    MEMORY_TRACE_T memoryTrace;
    top = initMemoryTrace(lcd.width, 80, top, &memoryTrace);

    //---------------------------------------------------------------------

    sleep(1);

    while (run)
    {
        struct timeval now;
        gettimeofday(&now, NULL);

        //-----------------------------------------------------------------

        showDynamicInfo(&lcd, &dynamicInfo);
        graphCpuUsage(now.tv_sec, &lcd, &cpuTrace);
        graphMemoryUsage(now.tv_sec, &lcd, &memoryTrace);

        //-----------------------------------------------------------------

        gettimeofday(&now, NULL);
        usleep(1000000L - now.tv_usec);
    }

    //---------------------------------------------------------------------

    destroyStaticInfo(&staticInfo);
    destroyDynamicInfo(&dynamicInfo);
    destroyCpuTrace(&cpuTrace);
    destroyMemoryTrace(&memoryTrace);

    closeLcd(&lcd);

    //---------------------------------------------------------------------

    messageLog(isDaemon, program, LOG_INFO, "exiting");

    if (isDaemon)
    {
        closelog();
    }

    //---------------------------------------------------------------------

    return 0 ;
}

