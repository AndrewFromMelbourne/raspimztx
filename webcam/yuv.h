//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2015 Andrew Duncan
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

#include "image.h"

#ifndef YUV_H
#define YUV_H

//-------------------------------------------------------------------------

typedef struct
{
    uint8_t y;
    uint8_t u;
    uint8_t v;
} YUV8_T;

typedef struct
{
    uint16_t lookup[32 * 32 * 32];
} YUV555_LOOKUP_T;

//-------------------------------------------------------------------------

uint16_t
packYUV555(
    uint8_t y,
    uint8_t u,
    uint8_t v);

//-------------------------------------------------------------------------

void
yuvToRgb(
    const YUV8_T *yuv,
    RGB8_T *rgb);

//-------------------------------------------------------------------------

void
initYUV555Lookup(
    YUV555_LOOKUP_T *lookup);

uint16_t
lookupYUVtoRGB565(
    const YUV555_LOOKUP_T *lookup,
    const YUV8_T *yuv);

uint16_t
lookupYUV555toRGB565(
    const YUV555_LOOKUP_T *lookup,
    uint16_t yuv);

//-------------------------------------------------------------------------

#endif
