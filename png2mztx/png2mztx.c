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

#include <getopt.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#include <png.h>

#include "image.h"
#include "key.h"
#include "lcd.h"
#include "loadpng.h"
#include "nearestNeighbour.h"

//-------------------------------------------------------------------------

const char* program = NULL;

//-------------------------------------------------------------------------

void
printUsage(
    const char *name)
{
    printf("usage: %s --file <file.jpg> ... options\n", name);
    printf("    --help - print this help message\n");
    printf("    --portrait - display in portrait orientation\n");
    printf("\n");
}

//-------------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    program = basename(argv[0]);
    char *filename = NULL;

    uint16_t rotate = 90;

    //---------------------------------------------------------------------

    static const char *sopts = "f:hp";
    static struct option lopts[] = 
    {
        { "file", required_argument, NULL, 'f' },
        { "help", no_argument, NULL, 'h' },
        { "portrait", no_argument, NULL, 'p' },
        { NULL, no_argument, NULL, 0 }
    };

    int opt = 0;

    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'f':

            filename = optarg;

            break;

        case 'h':

            printUsage(program);
            exit(EXIT_SUCCESS);

            break;

        case 'p':

            rotate = 0;

            break;

        default:

            printUsage(program);
            exit(EXIT_FAILURE);

            break;
        }
    }

    //---------------------------------------------------------------------

    if (filename == NULL)
    {
        printUsage(program);
        exit(EXIT_FAILURE);
    }

    if (getuid() != 0)
    {
        fprintf(stderr,
                "%s: you must be root to run this program\n",
                program);
        exit(EXIT_FAILURE);
    }

    LCD_T lcd;

    if (initLcd(&lcd, rotate) == false)
    {
        fprintf(stderr, "LCD initialization failed\n");
        exit(EXIT_FAILURE);
    }

    IMAGE_T image;
    if (loadPng(filename, &image) == false)
    {
        fprintf(stderr, "%s: failed to open %s\n", program, filename);
        exit(EXIT_FAILURE);
    }
    

    //---------------------------------------------------------------------


    if ((image.width > lcd.width) || (image.height > lcd.height))
    {
        NEAREST_NEIGHBOUR_T nn;

        initNearestNeighbour(&nn,
                             lcd.width,
                             lcd.height,
                             image.width,
                             image.height,
                             true);

        IMAGE_T resized;
        initImage(&resized,
                  nn.destinationWidth,
                  nn.destinationHeight,
                  false);

        resizeNearestNeighbour(&nn,
                               resized.buffer,
                               resized.width * sizeof(uint16_t),
                               image.buffer,
                               image.width * sizeof(uint16_t));

        putImageLcd(&lcd,
                    (lcd.width - resized.width) / 2,
                    (lcd.height - resized.height) / 2,
                    &resized);

        destroyImage(&resized);
    }
    else
    {
        putImageLcd(&lcd,
                    (lcd.width - image.width) / 2,
                    (lcd.height - image.height) / 2,
                    &image);
    }

    destroyImage(&image);

    int c = 0;
    while (c != 27)
    {
        usleep(100000);
        keyPressed(&c);
    }

    keyboardReset();
    closeLcd(&lcd);

    return 0 ;
}

