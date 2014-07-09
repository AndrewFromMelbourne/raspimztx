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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cpuTrace.h"
#include "font.h"
#include "image.h"
#include "lcd.h"

//-------------------------------------------------------------------------

static void
readCpuStats(
    CPU_STATS_T *cpuStats)
{
    FILE *fp = fopen("/proc/stat", "r");

    if (fp == NULL)
    {
        perror("unable to open /proc/stat");
        exit(EXIT_FAILURE);
    }

    fscanf(fp, "%*s");
    fscanf(fp, "%"SCNu32, &(cpuStats->user));
    fscanf(fp, "%"SCNu32, &(cpuStats->nice));
    fscanf(fp, "%"SCNu32, &(cpuStats->system));
    fscanf(fp, "%"SCNu32, &(cpuStats->idle));
    fscanf(fp, "%"SCNu32, &(cpuStats->iowait));
    fscanf(fp, "%"SCNu32, &(cpuStats->irq));
    fscanf(fp, "%"SCNu32, &(cpuStats->softirq));
    fscanf(fp, "%"SCNu32, &(cpuStats->steal));
    fscanf(fp, "%"SCNu32, &(cpuStats->guest));
    fscanf(fp, "%"SCNu32, &(cpuStats->guest_nice));

    fclose(fp);
}

//-------------------------------------------------------------------------

static void
diffCpuStats(
    CPU_STATS_T *a,
    CPU_STATS_T *b,
    CPU_STATS_T *res)
{
    res->user = a->user - b->user;
    res->nice = a->nice - b->nice;
    res->system = a->system - b->system;
    res->idle = a->idle - b->idle;
    res->iowait = a->iowait - b->iowait;
    res->irq = a->irq - b->irq;
    res->softirq = a->softirq - b->softirq;
    res->steal = a->steal - b->steal;
    res->guest = a->guest - b->guest;
    res->guest_nice = a->guest_nice - b->guest_nice;
}

//-------------------------------------------------------------------------

void
initCpuTrace(
    int16_t width,
    int16_t traceHeight,
    int16_t yPosition,
    CPU_TRACE_T *trace)
{
    int16_t height = traceHeight + FONT_HEIGHT + 4;

    trace->traceHeight = traceHeight;
    trace->yPosition = yPosition;

    trace->values = 0;

    trace->user = calloc(1, width);

    if (trace->user == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    trace->nice = calloc(1, width);

    if (trace->nice == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    trace->system = calloc(1, width);

    if (trace->system == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    trace->time = calloc(1, width);

    if (trace->time == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    if (initImage(&(trace->image), width, height, true) == false)
    {
        exit(EXIT_FAILURE);
    }

    setRGB(&(trace->userColour), 4, 90, 141);
    setRGB(&(trace->niceColour), 116, 169, 207);
    setRGB(&(trace->systemColour), 241, 238, 246);
    setRGB(&(trace->foreground), 255, 255, 255);
    setRGB(&(trace->background), 0, 0, 0);
    setRGB(&(trace->gridColour), 48, 48, 48);

    blendRGB(63,
             &(trace->gridColour),
             &(trace->userColour),
             &(trace->userGridColour));

    blendRGB(63,
             &(trace->gridColour),
             &(trace->niceColour),
             &(trace->niceGridColour));

    blendRGB(63,
             &(trace->gridColour),
             &(trace->systemColour),
             &(trace->systemGridColour));

    //---------------------------------------------------------------------

    IMAGE_T *image = &(trace->image);

    clearImageRGB(image, &(trace->background));

    FONT_POSITION_T position = 
        drawStringRGB(0,
                      image->height - 2 - FONT_HEIGHT,
                      "CPU",
                      &(trace->foreground),
                      image);

    position = drawStringRGB(position.x,
                             position.y,
                             " (user:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->userColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y,
                             " nice:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->niceColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y,
                             " system:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->systemColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y,
                             ")",
                             &(trace->foreground),
                             image);

    int32_t j = 0;
    for (j = 0 ; j < traceHeight + 1 ; j+= 20)
    {
        int32_t i = 0;
        for (i = 0 ; i < image->width ;  i++)
        {
            setPixelRGB(image, i, j, &(trace->gridColour));
        }
    }

    //---------------------------------------------------------------------

    readCpuStats(&(trace->currentStats));
}

//-------------------------------------------------------------------------

void
destroyCpuTrace(
    CPU_TRACE_T *trace)
{
    free(trace->user);
    free(trace->nice);
    free(trace->system);
    free(trace->time);

    destroyImage(&(trace->image));
}

//-------------------------------------------------------------------------

void
graphCpuUsage(
    time_t now,
    LCD_T *lcd,
    CPU_TRACE_T *trace)
{
    IMAGE_T *image = &(trace->image);

    CPU_STATS_T diff;

    memcpy(&(trace->previousStats),
           &(trace->currentStats),
           sizeof(trace->previousStats));

    readCpuStats(&(trace->currentStats));

    diffCpuStats(&(trace->currentStats),
                 &(trace->previousStats),
                 &diff);

    uint32_t totalCpu = diff.user
                      + diff.nice
                      + diff.system
                      + diff.idle
                      + diff.iowait
                      + diff.irq
                      + diff.softirq
                      + diff.steal
                      + diff.guest
                      + diff.guest_nice;

    int8_t user = (diff.user * trace->traceHeight) / totalCpu;
    int8_t nice = (diff.nice * trace->traceHeight) / totalCpu;
    int8_t system = (diff.system * trace->traceHeight) / totalCpu;

    int16_t index;

    if (trace->values < image->width)
    {
        index = trace->values;

        trace->values += 1;
    }
    else
    {
        index = image->width - 1;

        memmove(&(trace->user[0]), &(trace->user[1]), index);
        memmove(&(trace->nice[0]), &(trace->nice[1]), index);
        memmove(&(trace->system[0]), &(trace->system[1]), index);
        memmove(&(trace->time[0]), &(trace->time[1]), index);
    }

    trace->user[index] = user;
    trace->nice[index] = nice;
    trace->system[index] = system;
    trace->time[index] = now % 60;

    //-----------------------------------------------------------------

    int16_t i = 0;
    for (i = 0 ; i < trace->values ; i++)
    {
        int16_t j = trace->traceHeight;

        int16_t u = 0;
        for (u = 0 ; u < trace->user[i] ; u++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->userGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->userColour));
            }
        }

        int16_t n = 0;
        for (n = 0 ; n < trace->nice[i] ; n++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->niceGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->niceColour));
            }
        }

        int16_t s = 0;
        for (s = 0 ; s < trace->system[i] ; s++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->systemGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->systemColour));
            }
        }

        for ( ; j >= 0 ; j--)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j, &(trace->gridColour));
            }
            else
            {
                setPixelRGB(image, i, j, &(trace->background));
            }
        }
    }

    putImageLcd(lcd, 0, trace->yPosition, image);
}

