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

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/time.h>

#include "bcm_host.h"

#include "lcd.h"
#include "syslogUtilities.h"

//-------------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-------------------------------------------------------------------------

#define DEFAULT_FRAME_DURATION 500000

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
    fprintf(stderr, "    --fps <fps> - set desired frames per second");
    fprintf(stderr,
			" (default %d frames per second)\n",
			1000000 / DEFAULT_FRAME_DURATION);
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
main(int argc, char *argv[])
{
    char *program = basename(argv[0]);

    suseconds_t frameDuration =  DEFAULT_FRAME_DURATION;
    bool isDaemon =  false;

    //---------------------------------------------------------------------

    static const char *sopts = "df:h";
    static struct option lopts[] = 
    {
        { "daemon", no_argument, NULL, 'd' },
        { "fps", required_argument, NULL, 'f' },
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

        case 'f':
        {
            int fps = atoi(optarg);

            if (fps > 0)
            {
                frameDuration = 1000000 / fps;
            }

            break;
        }
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

    if (initLcd(&lcd, true) == false)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "LCD initialization failed");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGB565;
    int bytesPerPixel = 2;
    int width = lcd.width;
    int height = lcd.height;
    int pitch = bytesPerPixel * ALIGN_TO_16(width);

    void *dmxImagePtr = malloc(pitch * height);

    if (dmxImagePtr == NULL)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "unable to allocated image buffer");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    //---------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T displayHandle = vc_dispmanx_display_open(0);

    DISPMANX_MODEINFO_T modeInfo;

    int result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    //---------------------------------------------------------------------

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 width,
                                                 height,
                                                 &vcImagePtr);

    //---------------------------------------------------------------------

    struct timeval start_time;
    struct timeval end_time;
    struct timeval elapsed_time;

    //---------------------------------------------------------------------

    while (run)
    {
        gettimeofday(&start_time, NULL);

        //-----------------------------------------------------------------

        result = vc_dispmanx_snapshot(displayHandle,
                                      resourceHandle,
                                      DISPMANX_NO_ROTATE);

        if (result != 0)
        {
            free(dmxImagePtr);
            vc_dispmanx_resource_delete(resourceHandle);
            vc_dispmanx_display_close(displayHandle);

            messageLog(isDaemon,
                       program,
                       LOG_ERR,
                      "vc_dispmanx_snapshot() failed");
            exit(EXIT_FAILURE);
        }

        VC_RECT_T rect;
        result = vc_dispmanx_rect_set(&rect, 0, 0, width, height);

        result = vc_dispmanx_resource_read_data(resourceHandle,
                                                &rect,
                                                dmxImagePtr,
                                                pitch);

        if (result != 0)
        {
            free(dmxImagePtr);
            vc_dispmanx_resource_delete(resourceHandle);
            vc_dispmanx_display_close(displayHandle);

            messageLog(isDaemon,
                       program,
                       LOG_ERR,
                    "vc_dispmanx_resource_read_data() failed");
            exit(EXIT_FAILURE);
        }

        //-----------------------------------------------------------------

        putRGB565Lcd(&lcd,
                     0,
                     0,
                     width,
                     height,
                     pitch,
                     dmxImagePtr);

        //-----------------------------------------------------------------

        gettimeofday(&end_time, NULL);
        timersub(&end_time, &start_time, &elapsed_time);

        if (elapsed_time.tv_sec == 0)
        {
            if (elapsed_time.tv_usec < frameDuration)
            {
                usleep(frameDuration -  elapsed_time.tv_usec);
            }
        }
    }

    //---------------------------------------------------------------------

    clearLcd(&lcd, packRGB(0, 0, 0));
    closeLcd(&lcd);

    //---------------------------------------------------------------------

    free(dmxImagePtr);

    //---------------------------------------------------------------------

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    //---------------------------------------------------------------------

    messageLog(isDaemon, program, LOG_INFO, "exiting");

    if (isDaemon)
    {
        closelog();
    }

    //---------------------------------------------------------------------

    return 0 ;
}

