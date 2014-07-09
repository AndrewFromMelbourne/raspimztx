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
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "bcm_host.h"

#include "font.h"
#include "image.h"
#include "lcd.h"
#include "staticInfo.h"

//-------------------------------------------------------------------------

static void
getIpAddress(
    char *buffer, 
    size_t bufferSize)
{
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;

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
            }
        }
    }

    if (ifaddr != NULL)
    {
        freeifaddrs(ifaddr);
    }
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

void
initStaticInfo(
    int16_t width,
    int16_t yPosition,
    STATIC_INFO_T *info)
{
    info->yPosition = yPosition;

    RGB8_T heading = { 255, 255, 0 };
    RGB8_T foreground = { 255, 255, 255 };
    RGB8_T background = { 0, 0, 0 };

    //---------------------------------------------------------------------

    IMAGE_T *image = &(info->image);

    if (initImage(image, width, FONT_HEIGHT + 4, true) == false)
    {
        exit(EXIT_FAILURE);
    }

    clearImageRGB(image, &background);

    FONT_POSITION_T position = 
        drawStringRGB(0,
                      image->height - 2 - FONT_HEIGHT,
                      "ip: ",
                      &heading,
                      image);

    char ipaddress[INET_ADDRSTRLEN];
    getIpAddress(ipaddress, sizeof(ipaddress));

    position = drawStringRGB(position.x,
                             position.y,
                             ipaddress,
                             &foreground,
                             image);

    position = drawStringRGB(position.x,
                             position.y,
                             " memory: ",
                             &heading,
                             image);

    char memorySplit[10];
    getMemorySplit(memorySplit, sizeof(memorySplit));

    position = drawStringRGB(position.x,
                             position.y,
                             memorySplit,
                             &foreground,
                             image);

    position = drawStringRGB(position.x,
                             position.y,
                             " MB",
                             &foreground,
                             image);
}

//-------------------------------------------------------------------------

void
destroyStaticInfo(
    STATIC_INFO_T *info)
{
    destroyImage(&(info->image));
}

//-------------------------------------------------------------------------

void
showStaticInfo(
    LCD_T *lcd,
    STATIC_INFO_T *info)
{
    putImageLcd(lcd, 0, info->yPosition, &(info->image));
}

