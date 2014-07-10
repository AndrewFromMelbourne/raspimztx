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

#ifndef IMAGE_H
#define IMAGE_H

//-------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

//-------------------------------------------------------------------------

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB8_T;

//-------------------------------------------------------------------------

typedef struct IMAGE_T_ IMAGE_T;

struct IMAGE_T_
{
    int16_t width;
    int16_t height;
    int32_t size;
    uint16_t *buffer;
    void (*clearImage)(IMAGE_T*, const RGB8_T*);
    void (*setPixel)(IMAGE_T*, int16_t, int16_t, const RGB8_T*);
};

//-------------------------------------------------------------------------

void
setRGB(
    RGB8_T *rgb,
    uint8_t red,
    uint8_t green,
    uint8_t blue);

void
blendRGB(
    uint8_t alpha,
    const RGB8_T *a,
    const RGB8_T *b,
    RGB8_T *result);

//-------------------------------------------------------------------------

bool
initImage(
    IMAGE_T *image,
    int16_t width,
    int16_t height,
    bool dither);

void
clearImageRGB(
    IMAGE_T *image,
    const RGB8_T *rgb);

bool
setPixelRGB(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb);

bool
getPixelRGB(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    RGB8_T *rgb);

void
destroyImage(
    IMAGE_T *image);

//-------------------------------------------------------------------------

#endif
