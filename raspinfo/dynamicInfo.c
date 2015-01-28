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

static char
getIpAddress(
    char *buffer, 
    size_t bufferSize)
{
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;
    char interface = 'X';

    getifaddrs(&ifaddr);

    if (ifaddr == NULL)
    {
        snprintf(buffer, bufferSize, "   .   .   .   ");
    }

    for (ifa = ifaddr ; ifa != NULL ; ifa = ifa->ifa_next)
    {
        if (ifa ->ifa_addr->sa_family == AF_INET)
        {
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

            if (strcmp(ifa->ifa_name, "lo") != 0)
            {
                inet_ntop(AF_INET, addr, buffer, bufferSize);
                interface = ifa->ifa_name[0];
                break;
            }
        }
    }

    if (ifaddr != NULL)
    {
        freeifaddrs(ifaddr);
    }

    return interface;
}

//-------------------------------------------------------------------------

static void
getMemorySplit(
    char *buffer, 
    size_t bufferSize)
{
    int arm_mem = 0;
    int gpu_mem = 0;

    char response[128];

    memset(response, 0, sizeof(response));

    if (vc_gencmd(response, sizeof(response), "get_mem arm") == 0)
    {
        sscanf(response, "arm=%dM", &arm_mem);
    }

    if (vc_gencmd(response, sizeof(response), "get_mem gpu") == 0)
    {
        sscanf(response, "gpu=%dM", &gpu_mem);
    }

    if ((arm_mem != 0) && (gpu_mem != 0))
    {
        snprintf(buffer, bufferSize, "%d/%d", gpu_mem, arm_mem);
    }
    else
    {
        snprintf(buffer, bufferSize, " / ");
    }
}

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

int16_t
initDynamicInfo(
    int16_t width,
    int16_t yPosition,
    DYNAMIC_INFO_T *info)
{
    info->yPosition = yPosition;

    //---------------------------------------------------------------------

    IMAGE_T *image = &(info->image);

    if (initImage(image, width, 2 * (FONT_HEIGHT + 4), false) == false)
    {
        exit(EXIT_FAILURE);
    }

    info->heading = packRGB565(255, 255, 0);
    info->foreground = packRGB565(255, 255, 255);
    info->background = packRGB565(0, 0, 0);

    //---------------------------------------------------------------------

    return yPosition + image->height;
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

    clearImageRGB565(image, info->background);

    //---------------------------------------------------------------------

    FONT_POSITION_T position = { .x = 0, .y = 0 };

    position = drawStringRGB565(position.x,
                                position.y,
                                "ip(",
                                info->heading,
                                image);

    char ipaddress[INET_ADDRSTRLEN];
    char networkInterface = getIpAddress(ipaddress, sizeof(ipaddress));

    position = drawCharRGB565(position.x,
                              position.y,
                              networkInterface,
                              info->foreground,
                              image);

    position = drawStringRGB565(position.x,
                                position.y,
                                ") ",
                                info->heading,
                                image);

    position = drawStringRGB565(position.x,
                                position.y,
                                ipaddress,
                                info->foreground,
                                image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " memory ",
                                info->heading,
                                image);

    char memorySplit[10];
    getMemorySplit(memorySplit, sizeof(memorySplit));

    position = drawStringRGB565(position.x,
                                position.y,
                                memorySplit,
                                info->foreground,
                                image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " MB",
                                info->foreground,
                                image);

    //---------------------------------------------------------------------

    position.x = 0;
    position.y += FONT_HEIGHT + 4;

    position = drawStringRGB565(position.x,
                                position.y,
                                "time ",
                                info->heading,
                                image);

    char timeString[32];
    getTime(timeString, sizeof(timeString));

    position = drawStringRGB565(position.x,
                                position.y,
                                timeString,
                                info->foreground,
                                image);

    position = drawStringRGB565(position.x,
                                position.y,
                                " temperature ",
                                info->heading,
                                image);

    char temperatureString[10];
    getTemperature(temperatureString, sizeof(temperatureString));

    position = drawStringRGB565(position.x,
                                position.y,
                                temperatureString,
                                info->foreground,
                                image);

    uint8_t degreeSymbol = 0xF8;

    position = drawCharRGB565(position.x,
                              position.y,
                              degreeSymbol,
                              info->foreground,
                              image);


    position = drawStringRGB565(position.x,
                                position.y,
                                "C",
                                info->foreground,
                                image);

    putImageLcd(lcd, 0, info->yPosition, &(info->image));
}

