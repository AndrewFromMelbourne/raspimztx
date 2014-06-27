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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "image.h"

//-------------------------------------------------------------------------

void
setPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb);

void
setPixelDitheredRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb);

void
getPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    RGB8_T *rgb);

//-------------------------------------------------------------------------

bool initImage(
    IMAGE_T *image,
    int16_t width,
    int16_t height,
    bool dither)
{
    if (dither)
    {
        image->setPixel = setPixelDitheredRGB565;
    }
    else
    {
        image->setPixel = setPixelRGB565;
    }

    image->width = width;
    image->height = height;
    image->size = width * height * sizeof(uint16_t);

    image->buffer = calloc(1, image->size);

    if (image->buffer == NULL)
    {
        perror("image: memory exhausted\n");
        exit(EXIT_FAILURE);
    }

    return true;
}

//-------------------------------------------------------------------------

void
clearImageRGB(
    IMAGE_T *image,
    const RGB8_T *rgb)
{
    if (image->setPixel != NULL)
    {
        int j;
        for (j = 0 ; j < image->height ; j++)
        {
            int i;
            for (i = 0 ; i < image->width ; i++)
            {
                image->setPixel(image, i, j, rgb);
            }
        }
    }
}

//-------------------------------------------------------------------------

bool
setPixelRGB(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb)
{
    bool result = false;

    if ((image->setPixel != NULL) &&
        (x >= 0) && (x < image->width) &&
        (y >= 0) && (y < image->height))
    {
        result = true;
        image->setPixel(image, x, y, rgb);
    }

    return result;
}

//-------------------------------------------------------------------------

bool
getPixelRGB(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    RGB8_T *rgb)
{
    bool result = false;

    if ((x >= 0) && (x < image->width) &&
        (y >= 0) && (y < image->height))
    {
        result = true;
        getPixelRGB565(image, x, y, rgb);
    }

    return result;
}

//-------------------------------------------------------------------------

void
destroyImage(
    IMAGE_T *image)
{
    if (image->buffer)
    {
        free(image->buffer);
    }

    image->width = 0;
    image->height = 0;
    image->size = 0;
    image->buffer = NULL;
    image->setPixel = NULL;
}

//-----------------------------------------------------------------------

void
setPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb)
{
    uint8_t r5 = rgb->red >> 3;
    uint8_t g6 = rgb->green >> 2;
    uint8_t b5 = rgb->blue >> 3;

    uint16_t pixel = (r5 << 11) | (g6 << 5) | b5;
    image->buffer[x + (y * image->width)] = htons(pixel);
}

//-----------------------------------------------------------------------

void
setPixelDitheredRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb)
{
    static int16_t dither8[64] =
    {
        1, 6, 2, 7, 1, 6, 2, 7,
        4, 2, 5, 4, 4, 3, 6, 4,
        1, 7, 1, 6, 2, 7, 1, 7,
        5, 3, 5, 3, 5, 4, 5, 3,
        1, 6, 2, 7, 1, 6, 2, 7,
        4, 3, 6, 4, 4, 2, 6, 4,
        2, 7, 1, 7, 2, 7, 1, 6,
        5, 3, 5, 3, 5, 3, 5, 3,
    };

    static int16_t dither4[64] =
    {
        1, 3, 1, 3, 1, 3, 1, 3,
        2, 1, 3, 2, 2, 1, 3, 2,
        1, 3, 1, 3, 1, 3, 1, 3,
        2, 2, 2, 1, 3, 2, 2, 2,
        1, 3, 1, 3, 1, 3, 1, 3,
        2, 1, 3, 2, 2, 1, 3, 2,
        1, 3, 1, 3, 1, 3, 1, 3,
        3, 2, 2, 2, 2, 2, 2, 2,
    };

    int16_t index = (x & 7) | ((y & 7) << 3);

    int16_t r = rgb->red + dither8[index];

    if (r > 255)
    {
        r = 255;
    }

    int16_t g = rgb->green + dither4[index];

    if (g > 255)
    {
        g = 255;
    }

    int16_t b = rgb->blue + dither8[index];

    if (b > 255)
    {
        b = 255;
    }

    RGB8_T dithered = { r, g, b };

    setPixelRGB565(image, x, y, &dithered);
}

//-----------------------------------------------------------------------

void
getPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    RGB8_T *rgb)
{
    uint16_t pixel = image->buffer[x + (y * image->width)];
    pixel = ntohs(pixel);

    uint8_t r5 = (pixel >> 11) & 0x1F;
    uint8_t g6 = (pixel >> 5) & 0x3F;
    uint8_t b5 = pixel & 0x1F;

    rgb->red = (r5 << 3) | (r5 >> 2);
    rgb->green = (g6 << 2) | (g6 >> 4);
    rgb->blue = (b5 << 3) | (b5 >> 2);
}

