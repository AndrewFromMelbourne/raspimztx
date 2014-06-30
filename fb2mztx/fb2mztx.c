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
    const char *program = basename(argv[0]);

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

    char *fbdevice = "/dev/fb0";
    int fbfd = open(fbdevice, O_RDWR);

    if (fbfd == -1)
    {
        perrorLog(isDaemon, program, "cannot open framebuffer");
        exit(EXIT_FAILURE);
    }

    struct fb_fix_screeninfo finfo;

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "reading framebuffer fixed information");
        exit(EXIT_FAILURE);
    }

    struct fb_var_screeninfo vinfo;

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "reading framebuffer variabl information");
        exit(EXIT_FAILURE);
    }

    if (vinfo.bits_per_pixel != 16)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "only 16 bits per pixels supported");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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

    //---------------------------------------------------------------------

    return 0 ;
}

