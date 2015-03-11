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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "font.h"
#include "image.h"
#include "lcd.h"
#include "memoryTrace.h"

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

static void
getMemoryStats(
    MEMORY_STATS_T *memoryStats)
{
    FILE *fp = fopen("/proc/meminfo", "r");

    if (fp == NULL)
    {
        perror("unable to open /proc/meminfo");
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

int16_t
initMemoryTrace(
    int16_t width,
    int16_t traceHeight,
    int16_t yPosition,
    MEMORY_TRACE_T *trace)
{
    int16_t height = traceHeight + FONT_HEIGHT + 4;

    trace->traceHeight = traceHeight;
    trace->yPosition = yPosition;

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

    if (initImage(&(trace->image), width, height, true) == false)
    {
        exit(EXIT_FAILURE);
    }

    trace->usedColour = packRGB565(0, 109, 44);
    trace->buffersColour = packRGB565(102, 194, 164);
    trace->cachedColour = packRGB565(237, 248, 251);
    trace->foreground = packRGB565(255, 255, 255);
    trace->background = packRGB565(0, 0, 0);
    trace->gridColour = packRGB565(48, 48, 48);

    trace->usedGridColour = blendRGB565(63,
                                        trace->gridColour,
                                        trace->usedColour);

    trace->buffersGridColour = blendRGB565(63,
                                           trace->gridColour,
                                           trace->buffersColour);

    trace->cachedGridColour = blendRGB565(63,
                                          trace->gridColour,
                                          trace->cachedColour);

    //---------------------------------------------------------------------

    IMAGE_T *image = &(trace->image);

    clearImageRGB565(image, trace->background);

    uint8_t smallSquare = 0xFE;

    FONT_POSITION_T position = 
        drawStringRGB565(0,
                         image->height - 2 - FONT_HEIGHT,
                         "Memory",
                         trace->foreground,
                         image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " (used:",
                                trace->foreground,
                                image);

    position = drawCharRGB565(position.x,
                              position.y,
                              smallSquare,
                              trace->usedColour,
                              image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " buffers:",
                                trace->foreground,
                                image);

    position = drawCharRGB565(position.x,
                              position.y,
                              smallSquare,
                              trace->buffersColour,
                              image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " cached:",
                                trace->foreground,
                                image);

    position = drawCharRGB565(position.x,
                              position.y,
                              smallSquare,
                              trace->cachedColour,
                              image);

    position = drawStringRGB565(position.x,
                                position.y, ")",
                                trace->foreground,
                                image);

    int32_t j = 0;
    for (j = 0 ; j < traceHeight + 1 ; j+= 20)
    {
        int32_t i = 0;
        for (i = 0 ; i < image->width ;  i++)
        {
            setPixelRGB565(image, i, j, trace->gridColour);
        }
    }

    //---------------------------------------------------------------------

    return yPosition + height;
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
    getMemoryStats(&memoryStats);

    int16_t height = trace->traceHeight;

    int8_t used = (memoryStats.used * height) / memoryStats.total;
    int8_t buffers = (memoryStats.buffers * height) / memoryStats.total;
    int8_t cached = (memoryStats.cached * height) / memoryStats.total;

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
        int16_t j = height - 1;

        int16_t u = 0;
        for (u = 0 ; u < trace->used[i] ; u++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB565(image, i, j--, trace->usedGridColour);
            }
            else
            {
                setPixelRGB565(image, i, j--, trace->usedColour);
            }
        }

        int16_t b = 0;
        for (b = 0 ; b < trace->buffers[i] ; b++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB565(image, i, j--, trace->buffersGridColour);
            }
            else
            {
                setPixelRGB565(image, i, j--, trace->buffersColour);
            }
        }

        int16_t c = 0;
        for (c = 0 ; c < trace->cached[i] ; c++)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB565(image, i, j--, trace->cachedGridColour);
            }
            else
            {
                setPixelRGB565(image, i, j--, trace->cachedColour);
            }
        }

        for ( ; j >= 0 ; j--)
        {
            if (((j % 20) == 0) || (trace->time[i] == 0))
            {
                setPixelRGB565(image, i, j, trace->gridColour);
            }
            else
            {
                setPixelRGB565(image, i, j, trace->background);
            }
        }
    }

    putImageLcd(lcd, 0, trace->yPosition, &(trace->image));
}

