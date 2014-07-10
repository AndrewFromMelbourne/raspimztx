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

#include <ifaddrs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "bcm_host.h"

#include "font.h"
#include "image.h"
#include "lcd.h"
#include "dynamicInfo.h"

//-------------------------------------------------------------------------

static void
getTime(
    char *buffer, 
    size_t bufferSize)
{
    time_t t = time(NULL);
    struct tm result;
    struct tm *lt = localtime_r(&t, &result);
    strftime(buffer, bufferSize, "%T", lt);
}

//-------------------------------------------------------------------------

static void
getTemperature(
    char *buffer, 
    size_t bufferSize)
{
    double temperature = 0.0;

    char response[128];

    memset(response, 0, sizeof(response));

    if (vc_gencmd(response, sizeof(response), "measure_temp") == 0)
    {
        sscanf(response, "temp=%lf'C", &temperature);
    }

    snprintf(buffer, bufferSize, "%2.0f", temperature);
}

//-------------------------------------------------------------------------

void
initDynamicInfo(
    int16_t width,
    int16_t yPosition,
    DYNAMIC_INFO_T *info)
{
    info->yPosition = yPosition;

    //---------------------------------------------------------------------

    IMAGE_T *image = &(info->image);

    if (initImage(image, width, FONT_HEIGHT + 4, false) == false)
    {
        exit(EXIT_FAILURE);
    }

    setRGB(&(info->heading), 255, 255, 0);
    setRGB(&(info->foreground), 255, 255, 255);
    setRGB(&(info->background), 0, 0, 0);
}

//-------------------------------------------------------------------------

void
destroyDynamicInfo(
    DYNAMIC_INFO_T *info)
{
    destroyImage(&(info->image));
}

//-------------------------------------------------------------------------

void
showDynamicInfo(
    LCD_T *lcd,
    DYNAMIC_INFO_T *info)
{
    IMAGE_T *image = &(info->image);

    clearImageRGB(image, &(info->background));

    FONT_POSITION_T position = 
        drawStringRGB(0,
                      image->height - 2 - FONT_HEIGHT,
                      "time: ",
                      &(info->heading),
                      image);

    char timeString[32];
    getTime(timeString, sizeof(timeString));

    position = drawStringRGB(position.x,
                             position.y,
                             timeString,
                             &(info->foreground),
                             image);

    position = drawStringRGB(position.x,
                             position.y,
                             " temperature: ",
                             &(info->heading),
                             image);

    char temperatureString[10];
    getTemperature(temperatureString, sizeof(temperatureString));

    position = drawStringRGB(position.x,
                             position.y,
                             temperatureString,
                             &(info->foreground),
                             image);

    position = drawCharRGB(position.x,
                           position.y,
                           0xF8,
                           &(info->foreground),
                           image);


    position = drawStringRGB(position.x,
                             position.y,
                             "C",
                             &(info->foreground),
                             image);

    putImageLcd(lcd, 0, info->yPosition, &(info->image));
}

