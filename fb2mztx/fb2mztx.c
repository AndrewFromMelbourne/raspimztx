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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <bsd/libutil.h>

#include <linux/fb.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "lcd.h"
#include "nearestNeighbour.h"
#include "syslogUtilities.h"

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
    const char *program = basename(argv[0]);

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

    char *fbdevice = "/dev/fb0";
    int fbfd = open(fbdevice, O_RDWR);

    if (fbfd == -1)
    {
        perrorLog(isDaemon, program, "cannot open framebuffer");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    struct fb_fix_screeninfo finfo;

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "reading framebuffer fixed information");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    struct fb_var_screeninfo vinfo;

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "reading framebuffer variabl information");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    if (vinfo.bits_per_pixel != 16)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "only 16 bits per pixels supported");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    void *fbp = mmap(0,
                     finfo.smem_len,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fbfd,
                     0);

    if ((int)fbp == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "failed to map framebuffer device to memory");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    int32_t xOffset = 0;
    int32_t yOffset = 0;
    int32_t width = lcd.width;
    int32_t height = lcd.height;
    int32_t pitch = width * sizeof(uint16_t);

    //---------------------------------------------------------------------

    bool resize = ((width != vinfo.xres) ||
                   (height != vinfo.yres) ||
                   (pitch != finfo.line_length));

    NEAREST_NEIGHBOUR_T nn;

    if (resize)
    {
        initNearestNeighbour(&nn,
                             width,
                             height,
                             vinfo.xres,
                             vinfo.yres,
                             true);

        width = nn.destinationWidth;
        height = nn.destinationHeight;
        pitch = width * sizeof(uint16_t);

        xOffset = (lcd.width - width) / 2;
        yOffset = (lcd.height - height) / 2;
    }

    //---------------------------------------------------------------------

    void *fbcopy = calloc(1, pitch * height);

    if (fbcopy == NULL)
    {
        perrorLog(isDaemon, program, "failed to create copy buffer");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    struct timeval start_time;
    struct timeval end_time;
    struct timeval elapsed_time;

    //---------------------------------------------------------------------

    while (run)
    {
        gettimeofday(&start_time, NULL);

        //-----------------------------------------------------------------

        if (resize)
        {
            resizeNearestNeighbour(&nn,
                                   fbcopy,
                                   pitch,
                                   fbp,
                                   finfo.line_length);
        }
        else
        {
            memcpy(fbcopy, fbp, finfo.smem_len);
        }

        //-----------------------------------------------------------------

        putRGB565Lcd(&lcd,
                     xOffset,
                     yOffset,
                     width,
                     height,
                     width * sizeof(uint16_t),
                     fbcopy);

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

    //--------------------------------------------------------------------

    munmap(fbp, finfo.smem_len);
    close(fbfd);

    //---------------------------------------------------------------------

    free(fbcopy);

    //---------------------------------------------------------------------

    clearLcd(&lcd, packRGB(0, 0, 0));
    closeLcd(&lcd);

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

