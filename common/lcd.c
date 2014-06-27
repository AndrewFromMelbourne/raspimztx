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

#include <bcm2835.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "image.h"
#include "lcd.h"

//-------------------------------------------------------------------------

#define SPICS RPI_GPIO_P1_24
#define SPIRS RPI_GPIO_P1_22
#define SPIRST RPI_GPIO_P1_16

//-------------------------------------------------------------------------

inline static void
writeRegister(
    uint16_t value)
{
    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_clr(SPIRS);

    uint16_t network_value = htons(value);
    bcm2835_spi_writenb((char*)&network_value, 2);

    bcm2835_gpio_set(SPICS);
}

//-------------------------------------------------------------------------

inline static void
writeCommand(
    uint16_t index,
    uint16_t value)
{
    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_clr(SPIRS);

    uint16_t network_index = htons(index);
    bcm2835_spi_writenb((char*)&network_index, 2);

    bcm2835_gpio_set(SPIRS);

    uint16_t network_value = htons(value);
    bcm2835_spi_writenb((char*)&network_value, 2);

    bcm2835_gpio_set(SPICS);
}

//-------------------------------------------------------------------------

inline static void
writeData(
    uint16_t data)
{
    uint16_t network_data = htons(data);
    bcm2835_spi_writenb((char*)&network_data, 2);
}

//-------------------------------------------------------------------------

uint16_t
packRGB(
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    return (red >> 3) << 11 | (green >> 2) << 5 | (blue >> 3);
}

//-------------------------------------------------------------------------

bool
initLcd(
    LCD_T *lcd,
    bool rotate)
{
    if (rotate)
    {
        lcd->rotated = true;

        lcd->xStart = 0x0212;
        lcd->yStart = 0x0210;

        lcd->xEnd = 0x0213;
        lcd->yEnd = 0x0211;

        lcd->xPosition = 0x0201;
        lcd->yPosition = 0x0200;

        lcd->width = 320;
        lcd->height = 240;
    }
    else
    {
        lcd->rotated = false;

        lcd->xStart = 0x0210;
        lcd->yStart = 0x0212;

        lcd->xEnd = 0x0211;
        lcd->yEnd = 0x0213;

        lcd->xPosition = 0x0200;
        lcd->yPosition = 0x0201;

        lcd->width = 240;
        lcd->height = 320;
    }

    //---------------------------------------------------------------------

    if (bcm2835_init() == 0)
    {
        return false;
    }

    //---------------------------------------------------------------------

    bcm2835_gpio_fsel(RPI_GPIO_P1_12, BCM2835_GPIO_FSEL_ALT5);
    bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
    bcm2835_pwm_set_mode(0, 1, 1);
    bcm2835_pwm_set_range(0, 1024);

    //---------------------------------------------------------------------

    bcm2835_gpio_fsel(SPICS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(SPIRS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(SPIRST, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_spi_begin();

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2);

    bcm2835_gpio_clr(RPI_GPIO_P1_12);

    //---------------------------------------------------------------------

    bcm2835_gpio_clr(SPIRST);

    delay(100);

    bcm2835_gpio_set(SPIRST);

    delay(100);
    
    writeCommand(0x0000, 0x0001);

    delay(1);

    writeCommand(0x0100, 0x0000);
    writeCommand(0x0101, 0x0000);
    writeCommand(0x0102, 0x3110);
    writeCommand(0x0103, 0xE200);
    writeCommand(0x0110, 0x009D);
    writeCommand(0x0111, 0x0022);
    writeCommand(0x0100, 0x0120);

    delay(2);
    
    writeCommand(0x0100, 0x3120);

    delay(8);
    

    if (rotate)
    {
        writeCommand(0x0001, 0x0000);
    }
    else
    {
        writeCommand(0x0001, 0x0100);
    }

    writeCommand(0x0002, 0x0000);

    if (rotate)
    {
        writeCommand(0x0003, 0x12B8);
    }
    else
    {
        writeCommand(0x0003, 0x12B0);
    }

    writeCommand(0x0006, 0x0000);
    writeCommand(0x0007, 0x0101);
    writeCommand(0x0008, 0x0808);
    writeCommand(0x0009, 0x0000);
    writeCommand(0x000b, 0x0000);
    writeCommand(0x000c, 0x0000);
    writeCommand(0x000d, 0x0018);

    writeCommand(0x0012, 0x0000);
    writeCommand(0x0013, 0x0000);
    writeCommand(0x0018, 0x0000);
    writeCommand(0x0019, 0x0000);
    
    writeCommand(0x0203, 0x0000);
    writeCommand(0x0204, 0x0000);
    
    writeCommand(0x0210, 0x0000);
    writeCommand(0x0211, 0x00EF);
    writeCommand(0x0212, 0x0000);
    writeCommand(0x0213, 0x013F);
    writeCommand(0x0214, 0x0000);
    writeCommand(0x0215, 0x0000);
    writeCommand(0x0216, 0x0000);
    writeCommand(0x0217, 0x0000);
    
    writeCommand(0x0300, 0x5343);
    writeCommand(0x0301, 0x1021);
    writeCommand(0x0302, 0x0003);
    writeCommand(0x0303, 0x0011);
    writeCommand(0x0304, 0x050A);
    writeCommand(0x0305, 0x4342);
    writeCommand(0x0306, 0x1100);
    writeCommand(0x0307, 0x0003);
    writeCommand(0x0308, 0x1201);
    writeCommand(0x0309, 0x050A);
    
    writeCommand(0x0400, 0x4027);
    writeCommand(0x0401, 0x0000);
    writeCommand(0x0402, 0x0000);
    writeCommand(0x0403, 0x013F);
    writeCommand(0x0404, 0x0000);
    
    writeCommand(0x0200, 0x0000);
    writeCommand(0x0201, 0x0000);
    
    writeCommand(0x0100, 0x7120);

    writeCommand(0x0007, 0x0103);

    delay(1);

    writeCommand(0x0007, 0x0113);

    //---------------------------------------------------------------------

    clearLcd(lcd, 0);
    bcm2835_pwm_set_data(0, 0);

    //---------------------------------------------------------------------

    return true;
}

//-------------------------------------------------------------------------

void
closeLcd(
    LCD_T *lcd)
{
    // Turn diplay off
    writeCommand(0x0007, 0x0000);

    // Turn backlight off
    backlightLcd(1024);
    bcm2835_close();
}

//-------------------------------------------------------------------------

void
clearLcd(
    LCD_T *lcd,
    uint16_t rgb)
{
    writeCommand(lcd->xStart, 0x0000);
    writeCommand(lcd->yStart, 0x0000);

    writeCommand(lcd->xEnd, lcd->width - 1);
    writeCommand(lcd->yEnd, lcd->height - 1);
    
    writeCommand(lcd->xPosition, 0x0000);
    writeCommand(lcd->yPosition, 0x0000);
    
    writeRegister(0x0202);

    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_set(SPIRS);

    uint16_t buffer[lcd->width];
    uint16_t network_rgb = htons(rgb);

    int i;
    for (i = 0 ; i < lcd->width ; i++)
    {
        buffer[i] = network_rgb;
    }

    int j;
    for (j = 0 ; j < lcd->height ; j++)
    {
        bcm2835_spi_writenb((char*)buffer, sizeof(buffer));
    }

    bcm2835_gpio_set(SPICS);
}

//-------------------------------------------------------------------------

bool
filledBoxLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    uint16_t rgb)
{
    if ((width < 1) || (height < 1))
    {
        return false;
    }

    if (x >= lcd->width)
    {
        return false;
    }

    if (y >= lcd->height)
    {
        return false;
    }

    if (x < 0)
    {
        width += x;
        x = 0;
    }

    if (y < 0)
    {
        height += y;
        y = 0;
    }

    if ((x + width) > lcd->width)
    {
        width = lcd->width - x;
    }

    if ((y + height) > lcd->height)
    {
        height = lcd->height - y;
    }

    writeCommand(lcd->xStart, x);
    writeCommand(lcd->yStart, y);

    writeCommand(lcd->xEnd, x + width - 1);
    writeCommand(lcd->yEnd, y + height - 1);
    
    writeCommand(lcd->xPosition, 0x0000);
    writeCommand(lcd->yPosition, 0x0000);
    
    writeRegister(0x0202);

    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_set(SPIRS);

    uint16_t buffer[width];
    uint16_t network_rgb = htons(rgb);

    int i;
    for (i = 0 ; i < width ; i++)
    {
        buffer[i] = network_rgb;
    }

    int j;
    for (j = 0 ; j < height ; j++)
    {
        bcm2835_spi_writenb((char*)buffer, sizeof(buffer));
    }

    bcm2835_gpio_set(SPICS);

    return true;
}

//-------------------------------------------------------------------------

bool
setPixelLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    uint16_t rgb)
{
    if ((x < 0) || (x >= lcd->width))
    {
        return false;
    }

    if ((y < 0) || (y >= lcd->height))
    {
        return false;
    }

    writeCommand(lcd->xStart, x);
    writeCommand(lcd->yStart, y);
    writeCommand(lcd->xEnd, x);
    writeCommand(lcd->yEnd, y);

    writeCommand(lcd->xPosition, x);
    writeCommand(lcd->yPosition, y);

    writeCommand(0x202, rgb);

    return true;
}

//-------------------------------------------------------------------------

static bool
putImageLcdPartial(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    IMAGE_T *image)
{
    int16_t xStart = 0;
    int16_t xEnd = image->width - 1;

    int16_t yStart = 0;
    int16_t yEnd = image->height - 1;

    if (x < 0)
    {
        xStart = -x;
        x = 0;
    }

    if ((x - xStart + image->width) > lcd->width)
    {
        xEnd = lcd->width - 1 - (x - xStart);
    }

    if (y < 0)
    {
        yStart = -y;
        y = 0;
    }

    if ((y - yStart + image->height) > lcd->height)
    {
        yEnd = lcd->height - 1 - (y - yStart);
    }

    if ((xEnd - xStart) <= 0)
    {
        return false;
    }

    if ((yEnd - yStart) <= 0)
    {
        return false;
    }

    writeCommand(lcd->xStart, x);
    writeCommand(lcd->yStart, y);

    writeCommand(lcd->xEnd, x + xEnd - xStart);
    writeCommand(lcd->yEnd, y + yEnd - yStart);
    
    writeCommand(lcd->xPosition, 0x0000);
    writeCommand(lcd->yPosition, 0x0000);
    
    writeRegister(0x0202);

    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_set(SPIRS);

    uint32_t rowSize = (xEnd - xStart + 1) * sizeof(uint16_t); 

    int16_t j;
    for (j = yStart ; j <= yEnd ; j++)
    {
        uint16_t *row = &(image->buffer[xStart + (j * image->width)]);
        bcm2835_spi_writenb((char*)row, rowSize);
    }

    bcm2835_gpio_set(SPICS);

    return true;
}

//-------------------------------------------------------------------------

bool
putImageLcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    IMAGE_T *image)
{
    if ((x < 0) || ((x + image->width) > lcd->width))
    {
        return putImageLcdPartial(lcd, x, y, image);
    }

    if ((y < 0) || ((y + image->height) > lcd->height))
    {
        return putImageLcdPartial(lcd, x, y, image);
    }

    writeCommand(lcd->xStart, x);
    writeCommand(lcd->yStart, y);

    writeCommand(lcd->xEnd, x + image->width - 1);
    writeCommand(lcd->yEnd, y + image->height - 1);
    
    writeCommand(lcd->xPosition, 0x0000);
    writeCommand(lcd->yPosition, 0x0000);
    
    writeRegister(0x0202);

    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_set(SPIRS);

    bcm2835_spi_writenb((char*)image->buffer, image->size);

    bcm2835_gpio_set(SPICS);

    return true;
}

//-------------------------------------------------------------------------

bool
putRGB565Lcd(
    LCD_T *lcd,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    int16_t pitch,
    void *data)
{
    int16_t xStart = 0;
    int16_t xEnd = width - 1;

    int16_t yStart = 0;
    int16_t yEnd = height - 1;

    if (x < 0)
    {
        xStart = -x;
        x = 0;
    }

    if ((x - xStart + width) > lcd->width)
    {
        xEnd = lcd->width - 1 - (x - xStart);
    }

    if (y < 0)
    {
        yStart = -y;
        y = 0;
    }

    if ((y - yStart + height) > lcd->height)
    {
        yEnd = lcd->height - 1 - (y - yStart);
    }

    if ((xEnd - xStart) <= 0)
    {
        return false;
    }

    if ((yEnd - yStart) <= 0)
    {
        return false;
    }

    writeCommand(lcd->xStart, x);
    writeCommand(lcd->yStart, y);

    writeCommand(lcd->xEnd, x + xEnd - xStart);
    writeCommand(lcd->yEnd, y + yEnd - yStart);
    
    writeCommand(lcd->xPosition, 0x0000);
    writeCommand(lcd->yPosition, 0x0000);
    
    writeRegister(0x0202);

    bcm2835_gpio_clr(SPICS);
    bcm2835_gpio_set(SPIRS);

    uint32_t rowLength = xEnd - xStart + 1; 

    int16_t j = 0;
    for (j = yStart ; j <= yEnd ; j++)
    {
        uint16_t *row = data + (xStart * 2) + (j * pitch);

        int16_t i = 0;
        for (i = 0 ; i < rowLength ; i++)
        {
            row[i] = htons(row[i]);
        }

        bcm2835_spi_writenb((char*)row, rowLength * 2);
    }

    bcm2835_gpio_set(SPICS);

    return true;
}

//-------------------------------------------------------------------------

void
backlightLcd(
    uint32_t value)
{
    bcm2835_pwm_set_data(0, value);
}

//-------------------------------------------------------------------------
