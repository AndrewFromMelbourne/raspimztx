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

#ifndef NEAREST_NEIGHBOUR_H
#define NEAREST_NEIGHBOUR_H

//-------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//-------------------------------------------------------------------------

typedef struct
{
    int16_t destinationWidth;
    int16_t destinationHeight;
    int16_t sourceWidth;
    int16_t sourceHeight;
    int32_t xRatio;
    int32_t yRatio;
} NEAREST_NEIGHBOUR_T;

//-------------------------------------------------------------------------

void
initNearestNeighbour(
    NEAREST_NEIGHBOUR_T *nn,
    int16_t dWidth,
    int16_t dHeight,
    int16_t sWidth,
    int16_t sHeight,
    bool keepAspectRatio);

void
resizeNearestNeighbour(
    NEAREST_NEIGHBOUR_T *nn,
    void *dst,
    int16_t dPitch,
    void *src,
    int16_t sPitch);

//-------------------------------------------------------------------------

#endif

