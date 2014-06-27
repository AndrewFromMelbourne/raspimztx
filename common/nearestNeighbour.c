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

#include <stdbool.h>
#include <stdint.h>

#include "nearestNeighbour.h"

//-------------------------------------------------------------------------

void
initNearestNeighbour(
    NEAREST_NEIGHBOUR_T *nn,
    int16_t dWidth,
    int16_t dHeight,
    int16_t sWidth,
    int16_t sHeight,
    bool keepAspectRatio)
{
    int16_t width = dWidth;
    int16_t height = dHeight;

    nn->xRatio = (((int32_t)sWidth << 16) / width) + 1;
    nn->yRatio = (((int32_t)sHeight << 16) / height) + 1;

	nn->destinationWidth = width;
	nn->destinationHeight = height;

    if (keepAspectRatio)
    {
        if (nn->xRatio < nn->yRatio)
        {
            nn->xRatio = nn->yRatio;
            nn->destinationWidth = (sWidth * dHeight) / sHeight;
        }
        else
        {
            nn->yRatio = nn->xRatio;
            nn->destinationHeight = (dWidth * sHeight) / sWidth;
        }

    }

    nn->sourceWidth = sWidth;
    nn->sourceHeight = sHeight;
}

//-------------------------------------------------------------------------

void
resizeNearestNeighbour(
    NEAREST_NEIGHBOUR_T *nn,
    void *dst,
    int16_t dPitch,
    void *src,
    int16_t sPitch)
{
    int32_t j;
    for (j = 0 ; j < nn->destinationHeight ; j++)
    {
        int32_t y = (j * nn->yRatio) >> 16;

        int32_t i;
        for (i = 0 ; i < nn->destinationWidth ; i++)
        {
            int32_t x = (i * nn->xRatio) >> 16;

            *((uint16_t*)(dst + (i * sizeof(uint16_t)) + (j * dPitch))) =
            *((uint16_t*)(src + (x * sizeof(uint16_t)) + (y * sPitch)));
        }
    }
}

//-------------------------------------------------------------------------

