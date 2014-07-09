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

#ifndef MEMORY_TRACE_H
#define MEMORY_TRACE_H

//-------------------------------------------------------------------------

#include <stdint.h>

#include "image.h"
#include "lcd.h"

//-------------------------------------------------------------------------

typedef struct
{
    int16_t traceHeight;
    int16_t yPosition;
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
initMemoryTrace(
    int16_t width,
    int16_t traceHeight,
    int16_t yPosition,
    MEMORY_TRACE_T *trace);

void
destroyMemoryTrace(
    MEMORY_TRACE_T *trace);

void
graphMemoryUsage(
    time_t now,
    LCD_T *lcd,
    MEMORY_TRACE_T *trace);

//-------------------------------------------------------------------------

#endif

