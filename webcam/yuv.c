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

#include "yuv.h"

//-------------------------------------------------------------------------

uint16_t
packYUV555(
    uint8_t y,
    uint8_t u,
    uint8_t v)
{
    uint8_t y5 = y >> 3;
    uint8_t u5 = u >> 3;
    uint8_t v5 = v >> 3;

    return (y5 << 10) | (u5 << 5) | v5;
}

//-------------------------------------------------------------------------

static uint8_t
clip255(
    int16_t value)
{
    if (value < 0)
    {
        value = 0;
    }
    else if (value > 255)
    {
        value = 255;
    }

    return value;
}

//-------------------------------------------------------------------------

void
yuvToRgb(
    const YUV8_T *yuv,
    RGB8_T *rgb)
{
    int16_t c = yuv->y - 16;
    int16_t d = yuv->u - 128;
    int16_t e = yuv->v - 128;

    rgb->red = clip255((298 * c + 409 * e + 128) >> 8);
    rgb->green = clip255((298 * c - 100 * d - 208 * e + 128) >> 8);
    rgb->blue = clip255((298 * c + 516 * d + 128) >> 8);
}

//-------------------------------------------------------------------------

void
initYUV555Lookup(
    YUV555_LOOKUP_T *lookup)
{
    int16_t v = 0;
    for (v = 0 ; v < 32 ; v++)
    {
        int16_t u = 0;
        for (u = 0 ; u < 32 ; u++)
        {
            int16_t y = 0;
            for (y = 0 ; y < 32 ; y++)
            {
                YUV8_T yuv;
                RGB8_T rgb;

                yuv.y = (y << 3) | (y >> 2);
                yuv.u = (u << 3) | (u >> 2);
                yuv.v = (v << 3) | (v >> 2);

                yuvToRgb(&yuv, &rgb);

                uint16_t yuvPacked = packYUV555(yuv.y, yuv.u, yuv.v);

                lookup->lookup[yuvPacked] = packRGB565(rgb.red,
                                                       rgb.green,
                                                       rgb.blue);
            }
        }
    }
}

//-------------------------------------------------------------------------

uint16_t
lookupYUVtoRGB565(
    const YUV555_LOOKUP_T *lookup,
    const YUV8_T *yuv)
{
    return lookup->lookup[packYUV555(yuv->y, yuv->u, yuv->v)];
}

//-------------------------------------------------------------------------

uint16_t
lookupYUV555toRGB565(
    const YUV555_LOOKUP_T *lookup,
    uint16_t yuv)
{
    return lookup->lookup[yuv];
}

