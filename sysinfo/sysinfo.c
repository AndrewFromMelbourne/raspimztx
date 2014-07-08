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
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "font.h"
#include "image.h"
#include "key.h"
#include "lcd.h"

//-------------------------------------------------------------------------

typedef struct
{
    uint32_t user;
    uint32_t nice;
    uint32_t system;
    uint32_t idle;
    uint32_t iowait;
    uint32_t irq;
    uint32_t softirq;
    uint32_t steal;
    uint32_t guest;
    uint32_t guest_nice;
} CPU_STATS_T;

//-------------------------------------------------------------------------

typedef struct
{
    int16_t values;
    int8_t *user;
    int8_t *nice;
    int8_t *system;
    int8_t *time;
    CPU_STATS_T currentStats;
    CPU_STATS_T previousStats;
    IMAGE_T image;
    RGB8_T userColour;
    RGB8_T userGridColour;
    RGB8_T niceColour;
    RGB8_T niceGridColour;
    RGB8_T systemColour;
    RGB8_T systemGridColour;
    RGB8_T foreground;
    RGB8_T background;
    RGB8_T gridColour;
} CPU_TRACE_T;

//-------------------------------------------------------------------------

typedef struct
{
    uint32_t total;
    uint32_t free;
    uint32_t buffers;
    uint32_t cached;
    uint32_t used;
} MEMORY_STATS_T;

//-------------------------------------------------------------------------

typedef struct
{
    uint16_t values;
    int8_t *used;
    int8_t *buffers;
    int8_t *cached;
    int8_t *time;
    IMAGE_T image;
    RGB8_T usedColour;
    RGB8_T usedGridColour;
    RGB8_T buffersColour;
    RGB8_T buffersGridColour;
    RGB8_T cachedColour;
    RGB8_T cachedGridColour;
    RGB8_T foreground;
    RGB8_T background;
    RGB8_T gridColour;
} MEMORY_TRACE_T;

//-------------------------------------------------------------------------

void
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

void
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
    CPU_TRACE_T *trace)
{
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

    if (initImage(&(trace->image), width, 120, true) == false)
    {
        exit(EXIT_FAILURE);
    }

    setRGB(&(trace->userColour), 55, 126, 184);
    setRGB(&(trace->niceColour), 77, 175, 74);
    setRGB(&(trace->systemColour), 228, 26, 28);
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
                      image->height - 1 - FONT_HEIGHT,
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
    for (j = 0 ; j < 101 ; j+= 20)
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

    int8_t user = (diff.user * 100) / totalCpu;
    int8_t nice = (diff.nice * 100) / totalCpu;
    int8_t system = (diff.system * 100) / totalCpu;

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
        int16_t j = 99;

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

    putImageLcd(lcd, 0, 0, image);
}

//-------------------------------------------------------------------------

void
readMemoryStats(
    MEMORY_STATS_T *memoryStats)
{
    FILE *fp = fopen("/proc/meminfo", "r");

    if (fp == NULL)
    {
        perror("unable to open /proc/stat");
        exit(EXIT_FAILURE);
    }

    char buffer[128];

    while (fgets(buffer, sizeof(buffer), fp))
    {
        char name[64];
        uint32_t value;

        if (sscanf(buffer, "%[a-zA-Z]: %"SCNu32" kB", name, &value) == 2)
        {
            if (strcmp(name, "MemTotal") == 0)
            {
                memoryStats->total = value;
            }
            else if (strcmp(name, "MemFree") == 0)
            {
                memoryStats->free = value;
            }
            else if (strcmp(name, "Buffers") == 0)
            {
                memoryStats->buffers = value;
            }
            else if (strcmp(name, "Cached") == 0)
            {
                memoryStats->cached = value;
            }
        }
    }

    fclose(fp);

    memoryStats->used = memoryStats->total
                      - memoryStats->free
                      - memoryStats->buffers
                      - memoryStats->cached;
}

//-------------------------------------------------------------------------

void
initMemoryTrace(
    int16_t width,
    MEMORY_TRACE_T *trace)
{
    trace->values = 0;

    trace->used = calloc(1, width);

    if (trace->used == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    trace->buffers = calloc(1, width);

    if (trace->buffers == NULL)
    {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    trace->cached = calloc(1, width);

    if (trace->cached == NULL)
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

    if (initImage(&(trace->image), width, 120, true) == false)
    {
        exit(EXIT_FAILURE);
    }

    setRGB(&(trace->usedColour), 55, 126, 184);
    setRGB(&(trace->buffersColour), 77, 175, 74);
    setRGB(&(trace->cachedColour), 152, 78, 163);
    setRGB(&(trace->foreground), 255, 255, 255);
    setRGB(&(trace->background), 0, 0, 0);
    setRGB(&(trace->gridColour), 48, 48, 48);

    blendRGB(63,
             &(trace->gridColour),
             &(trace->usedColour),
             &(trace->usedGridColour));

    blendRGB(63,
             &(trace->gridColour),
             &(trace->buffersColour),
             &(trace->buffersGridColour));

    blendRGB(63,
             &(trace->gridColour),
             &(trace->cachedColour),
             &(trace->cachedGridColour));

    //---------------------------------------------------------------------

    IMAGE_T *image = &(trace->image);

    clearImageRGB(image, &(trace->background));

    FONT_POSITION_T position = 
        drawStringRGB(0,
                      image->height - 1 - FONT_HEIGHT,
                      "Memory",
                      &(trace->foreground),
                      image);

    position = drawStringRGB(position.x,
                             position.y,
                             " (used:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->usedColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y,
                             " buffers:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->buffersColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y,
                             " cached:",
                             &(trace->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xFE,
                           &(trace->cachedColour),
                           image);

    position = drawStringRGB(position.x,
                             position.y, ")",
                             &(trace->foreground),
                             image);

    int32_t j = 0;
    for (j = 0 ; j < 101 ; j+= 20)
    {
        int32_t i = 0;
        for (i = 0 ; i < image->width ;  i++)
        {
            setPixelRGB(image, i, j, &(trace->gridColour));
        }
    }
}

//-------------------------------------------------------------------------

void
destroyMemoryTrace(
    MEMORY_TRACE_T *trace)
{
    free(trace->used);
    free(trace->buffers);
    free(trace->cached);
    free(trace->time);

    destroyImage(&(trace->image));
}

//-------------------------------------------------------------------------

void
graphMemoryUsage(
    time_t now,
    LCD_T *lcd,
    MEMORY_TRACE_T *trace)
{
    MEMORY_STATS_T memoryStats = { 0, 0, 0, 0 };
    readMemoryStats(&memoryStats);

    int8_t used = (memoryStats.used * 100) / memoryStats.total;
    int8_t buffers = (memoryStats.buffers * 100) / memoryStats.total;
    int8_t cached = (memoryStats.cached * 100) / memoryStats.total;

    IMAGE_T *image = &(trace->image);

    int16_t index;

    if (trace->values < image->width)
    {
        index = trace->values;

        trace->values += 1;
    }
    else
    {
        index = image->width - 1;

        memmove(&(trace->used[0]), &(trace->used[1]), index);
        memmove(&(trace->buffers[0]), &(trace->buffers[1]), index);
        memmove(&(trace->cached[0]), &(trace->cached[1]), index);
        memmove(&(trace->time[0]), &(trace->time[1]), index);
    }

    trace->used[index] = used;
    trace->buffers[index] = buffers;
    trace->cached[index] = cached;
    trace->time[index] = now % 60;

    //-----------------------------------------------------------------

    int16_t i = 0;
    for (i = 0 ; i < trace->values ; i++)
    {
        int16_t j = 99;

        int16_t u = 0;
        for (u = 0 ; u < trace->used[i] ; u++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->usedGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->usedColour));
            }
        }

        int16_t b = 0;
        for (b = 0 ; b < trace->buffers[i] ; b++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->buffersGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->buffersColour));
            }
        }

        int16_t c = 0;
        for (c = 0 ; c < trace->cached[i] ; c++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB(image, i, j--, &(trace->cachedGridColour));
            }
            else
            {
                setPixelRGB(image, i, j--, &(trace->cachedColour));
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

    putImageLcd(lcd, 0, 120, &(trace->image));
}

//-------------------------------------------------------------------------

const char* program = NULL;

//-------------------------------------------------------------------------

int
main(
    int argc,
    char *argv[])
{
    program = basename(argv[0]);

    //---------------------------------------------------------------------

    if (getuid() != 0)
    {
        fprintf(stderr,
                "%s: you must be root to run this program\n",
                program);
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    LCD_T lcd;

    if (initLcd(&lcd, 90) == false)
    {
        fprintf(stderr, "LCD initialization failed\n");
        exit(EXIT_FAILURE);
    }

    clearLcd(&lcd, packRGB(0, 0, 0));

    //---------------------------------------------------------------------

    CPU_TRACE_T cpuTrace;
    initCpuTrace(lcd.width, &cpuTrace);

    //---------------------------------------------------------------------

    MEMORY_TRACE_T memoryTrace;
    initMemoryTrace(lcd.width, &memoryTrace);

    //---------------------------------------------------------------------

    sleep(1);

    int c = 0;
    while (c != 27)
    {
        struct timeval now;
        gettimeofday(&now, NULL);

        //-----------------------------------------------------------------

        graphCpuUsage(now.tv_sec, &lcd, &cpuTrace);
        graphMemoryUsage(now.tv_sec, &lcd, &memoryTrace);

        //-----------------------------------------------------------------

        keyPressed(&c);

        //-----------------------------------------------------------------

        gettimeofday(&now, NULL);
        usleep(1000000L - now.tv_usec);
    }

    destroyCpuTrace(&cpuTrace);
    destroyMemoryTrace(&memoryTrace);

    keyboardReset();
    closeLcd(&lcd);

    return 0 ;
}

