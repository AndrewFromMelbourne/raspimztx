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

#include <jpeglib.h>

#include "image.h"
#include "key.h"
#include "lcd.h"
#include "nearestNeighbour.h"

//-------------------------------------------------------------------------

const char* program = NULL;

//-------------------------------------------------------------------------

static bool
readJpeg(
    const char *file,
    LCD_T *lcd,
    IMAGE_T *image)
{
    FILE *fpin = fopen(file, "rb");

    if (fpin == NULL)
    {
        perror("Error: cannot open file");
        return false;
    }

    //---------------------------------------------------------------------

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fpin);
    jpeg_read_header(&cinfo, TRUE);

    //---------------------------------------------------------------------

    jpeg_calc_output_dimensions(&cinfo);

    double xratio = (double)(cinfo.output_width) / lcd->width;
    double yratio = (double)(cinfo.output_height) / lcd->height;

    double ratio = xratio;

    if (yratio > xratio)
    {
        ratio = yratio;
    }

    if (ratio > (8.0/1.0))
    {
        cinfo.scale_num = 1;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/2.0))
    {
        cinfo.scale_num = 2;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/3.0))
    {
        cinfo.scale_num = 3;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/4.0))
    {
        cinfo.scale_num = 4;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/5.0))
    {
        cinfo.scale_num = 5;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/6.0))
    {
        cinfo.scale_num = 6;
        cinfo.scale_denom = 8;
    }
    else if (ratio > (8.0/7.0))
    {
        cinfo.scale_num = 7;
        cinfo.scale_denom = 8;
    }
    else
    {
        cinfo.scale_num = 8;
        cinfo.scale_denom = 8;
    }

    jpeg_calc_output_dimensions(&cinfo);

    //---------------------------------------------------------------------

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    //---------------------------------------------------------------------

    bool result = initImage(image,
                            cinfo.output_width,
                            cinfo.output_height,
                            true);

    if (result == false)
    {
        return false;
    }

    //---------------------------------------------------------------------

    uint8_t *row = malloc(cinfo.output_width * 3);

    if (row == NULL)
    {
        perror("Error: cannot allocate read buffer");
        destroyImage(image);
        return false;
    }

    //---------------------------------------------------------------------

    int j = 0;
    for (j = 0 ; j < cinfo.output_height ; j++)
    {
        jpeg_read_scanlines(&cinfo, &row, 1);

        int i = 0;
        for (i = 0 ; i < cinfo.output_width ; i++)
        {
            uint8_t *pixel = row + (i * 3);

            RGB8_T rgb = { pixel[0], pixel[1], pixel[2] };
            setPixelRGB(image, i, j, &rgb);        
        }
    }

    //---------------------------------------------------------------------

    free(row);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(fpin);

    //---------------------------------------------------------------------

    return true;
}

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
main(
    int argc,
    char *argv[])
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
    if (readJpeg(filename, &lcd, &image) == false)
    {
        fprintf(stderr, "%s: failed to open %s\n", program, filename);
        exit(EXIT_FAILURE);
    }
    

    //---------------------------------------------------------------------


    if ((image.width != lcd.width) || (image.height != lcd.height))
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
        putImageLcd(&lcd, 0, 0, &image);
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

