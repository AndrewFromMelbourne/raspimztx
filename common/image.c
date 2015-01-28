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
clearImageDirect(
    IMAGE_T *image,
    const RGB8_T *rgb);

void
clearImageDithered(
    IMAGE_T *image,
    const RGB8_T *rgb);

void
setPixelDirect(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb);

void
setPixelDithered(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb);

void
getPixelDirect(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    RGB8_T *rgb);

//-------------------------------------------------------------------------

uint16_t
packRGB565(
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    uint8_t r5 = red >> 3;
    uint8_t g6 = green >> 2;
    uint8_t b5 = blue >> 3;

    uint16_t pixel = (r5 << 11) | (g6 << 5) | b5;

    return htons(pixel);
}

//-------------------------------------------------------------------------

void
setRGB(
    RGB8_T *rgb,
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    rgb->red = red;
    rgb->green = green;
    rgb->blue = blue;
}

//-------------------------------------------------------------------------

uint16_t
blendRGB565(
    uint8_t alpha,
    uint16_t a,
    uint16_t b)
{
    uint32_t value = ((a * alpha) + (b * (255-alpha))) / 255;
    return value;
}

//-------------------------------------------------------------------------

void
blendRGB(
    uint8_t alpha,
    const RGB8_T *a,
    const RGB8_T *b,
    RGB8_T *result)
{
    result->red = (((int16_t)(a->red) * alpha)
                + ((int16_t)(b->red) * (255 - alpha)))
                / 255;

    result->green = (((int16_t)(a->green) * alpha)
                  + ((int16_t)(b->green) * (255 - alpha)))
                  / 255;

    result->blue = (((int16_t)(a->blue) * alpha)
                 + ((int16_t)(b->blue) * (255 - alpha)))
                 / 255;
}

//-------------------------------------------------------------------------

bool initImage(
    IMAGE_T *image,
    int16_t width,
    int16_t height,
    bool dither)
{
    if (dither)
    {
        image->clearImage = clearImageDithered;
        image->setPixel = setPixelDithered;
    }
    else
    {
        image->clearImage = clearImageDirect;
        image->setPixel = setPixelDirect;
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
clearImageRGB565(
    IMAGE_T *image,
    uint16_t rgb)
{
    int length = image->width * image->height;
    uint16_t *buffer = image->buffer;

    int i = 0;
    for (i = 0 ; i < length ; i++)
    {
        *buffer++ = rgb;
    }
}

//-------------------------------------------------------------------------

void
clearImageRGB(
    IMAGE_T *image,
    const RGB8_T *rgb)
{
    if (image->clearImage != NULL)
    {
        image->clearImage(image, rgb);
    }
}

//-------------------------------------------------------------------------

void
clearImageDirect(
    IMAGE_T *image,
    const RGB8_T *rgb)
{
    uint16_t pixel = packRGB565(rgb->red, rgb->green, rgb->blue);

    int length = image->width * image->height;
    uint16_t *buffer = image->buffer;

    int i = 0;
    for (i = 0 ; i < length ; i++)
    {
        *buffer++ = pixel;
    }
}

//-------------------------------------------------------------------------

void
clearImageDithered(
    IMAGE_T *image,
    const RGB8_T *rgb)
{
    int j;
    for (j = 0 ; j < image->height ; j++)
    {
        int i;
        for (i = 0 ; i < image->width ; i++)
        {
            setPixelDithered(image, i, j, rgb);
        }
    }
}

//-------------------------------------------------------------------------

bool
setPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    uint16_t rgb)
{
    bool result = false;

    if ((image->setPixel != NULL) &&
        (x >= 0) && (x < image->width) &&
        (y >= 0) && (y < image->height))
    {
        result = true;
        image->buffer[x + (y * image->width)] = rgb;
    }

    return result;
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
getPixelRGB565(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    uint16_t *rgb)
{
    bool result = false;

    if ((x >= 0) && (x < image->width) &&
        (y >= 0) && (y < image->height))
    {
        result = true;
        *rgb = image->buffer[x + (y * image->width)];
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
        getPixelDirect(image, x, y, rgb);
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
setPixelDirect(
    IMAGE_T *image,
    int16_t x,
    int16_t y,
    const RGB8_T *rgb)
{
    image->buffer[x + (y * image->width)] = packRGB565(rgb->red,
                                                       rgb->green,
                                                       rgb->blue);
}

//-----------------------------------------------------------------------

void
setPixelDithered(
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

    setPixelDirect(image, x, y, &dithered);
}

//-----------------------------------------------------------------------

void
getPixelDirect(
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

