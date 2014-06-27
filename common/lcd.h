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

#ifndef LCD_H
#define LCD_H

//-------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "image.h"

//-------------------------------------------------------------------------

typedef struct
{
    uint16_t xStart;
    uint16_t yStart;
    uint16_t xEnd;
    uint16_t yEnd;
    uint16_t xPosition;
    uint16_t yPosition;
    uint16_t width;
    uint16_t height;
    bool rotated;
} LCD_T;

//-------------------------------------------------------------------------

uint16_t
packRGB(
    uint8_t red,
    uint8_t green,
    uint8_t blue);

bool
initLcd(
    LCD_T *lcd,
    bool rotate);

void
closeLcd(
    LCD_T *lcd);

void
clearLcd(
    LCD_T *lcd,
    uint16_t rgb);

bool
filledBoxLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    uint16_t rgb);

bool
setPixelLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    uint16_t rgb);

bool
putImageLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    IMAGE_T *image);

bool
putRGB565Lcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    int16_t pitch,
    void *data);

void
backlightLcd(
    uint32_t value);

//-------------------------------------------------------------------------

#endif
