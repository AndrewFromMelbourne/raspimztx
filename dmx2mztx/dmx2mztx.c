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

#include <bsd/libutil.h>

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
    FILE *fp,
    const char *name)
{
    fprintf(fp, "\n");
    fprintf(fp, "Usage: %s <options>\n", name);
    fprintf(fp, "\n");
    fprintf(fp, "    --daemon - start in the background as a daemon\n");
    fprintf(fp, "    --fps <fps> - set desired frames per second");
    fprintf(fp,
            " (default %d frames per second)\n",
            1000000 / DEFAULT_FRAME_DURATION);
    fprintf(fp, "    --pidfile <pidfile> - create and lock PID file (if being run as a daemon)\n");
    fprintf(fp, "    --help - print usage and exit\n");
    fprintf(fp, "\n");
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

    suseconds_t frameDuration =  DEFAULT_FRAME_DURATION;
    bool isDaemon =  false;
    char *pidfile = NULL;

    //---------------------------------------------------------------------

    static const char *sopts = "df:hp:";
    static struct option lopts[] = 
    {
        { "daemon", no_argument, NULL, 'd' },
        { "fps", required_argument, NULL, 'f' },
        { "help", no_argument, NULL, 'h' },
        { "pidfile", required_argument, NULL, 'p' },
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

            printUsage(stdout, program);
            exit(EXIT_SUCCESS);

            break;

        case 'p':

            pidfile = optarg;

            break;

        default:

            printUsage(stderr, program);
            exit(EXIT_FAILURE);

            break;
        }
    }

    //---------------------------------------------------------------------

    struct pidfh *pfh = NULL;

    if (isDaemon)
    {
        if (pidfile != NULL)
        {
            pid_t otherpid;
            pfh = pidfile_open(pidfile, 0600, &otherpid);

            if (pfh == NULL)
            {
                fprintf(stderr,
                        "%s is already running %jd\n",
                        program,
                        (intmax_t)otherpid);
                exit(EXIT_FAILURE);
            }
        }
        
        if (daemon(0, 0) == -1)
        {
            fprintf(stderr, "Cannot daemonize\n");
            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        if (pfh)
        {
            pidfile_write(pfh);
        }

        openlog(program, LOG_PID, LOG_USER);
    }

    //---------------------------------------------------------------------

    if (access("/dev/mem", R_OK | W_OK) == -1)
    {
        perrorLog(isDaemon,
                  program,
                 "read and write access to /dev/mem required");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    //---------------------------------------------------------------------
    //
    // Calling vc_dispmanx_snapshot() fails when the display is rotate
    // either 90 or 270 degrees. It sometimes causes the program to hang.
    // check the config to make sure the screen is not rotated.
    //

    char response[1024];
    int display_rotate = 0;

    if (vc_gencmd(response, sizeof(response), "get_config int") == 0)
    {
        vc_gencmd_number_property(response,
                                  "display_rotate",
                                  &display_rotate);
    }

    // only need to check low bit of display_rotate (value of 1 or 3).

    if (display_rotate & 1)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "cannot copy rotated display");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGINT signal handler");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGTERM signal handler");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    LCD_T lcd;

    if (initLcd(&lcd, 90) == false)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "LCD initialization failed");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
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
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

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
            exitAndRemovePidFile(EXIT_FAILURE, pfh);
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
            exitAndRemovePidFile(EXIT_FAILURE, pfh);
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

    if (pfh)
    {
        pidfile_remove(pfh);
    }

    //---------------------------------------------------------------------

    return 0 ;
}

