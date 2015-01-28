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

#ifndef CPU_TRACE_H
#define CPU_TRACE_H

//-------------------------------------------------------------------------

#include <stdint.h>

#include "image.h"
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
    int16_t traceHeight;
    int16_t yPosition;
    int16_t values;
    int8_t *user;
    int8_t *nice;
    int8_t *system;
    int8_t *time;
    CPU_STATS_T currentStats;
    CPU_STATS_T previousStats;
    IMAGE_T image;
    uint16_t userColour;
    uint16_t userGridColour;
    uint16_t niceColour;
    uint16_t niceGridColour;
    uint16_t systemColour;
    uint16_t systemGridColour;
    uint16_t foreground;
    uint16_t background;
    uint16_t gridColour;
} CPU_TRACE_T;

//-------------------------------------------------------------------------

int16_t
initCpuTrace(
    int16_t width,
    int16_t traceHeight,
    int16_t yPosition,
    CPU_TRACE_T *trace);

void
destroyCpuTrace(
    CPU_TRACE_T *trace);

void
graphCpuUsage(
    time_t now,
    LCD_T *lcd,
    CPU_TRACE_T *trace);

//-------------------------------------------------------------------------

#endif

