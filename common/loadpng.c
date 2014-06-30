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

#include <png.h>
#include <stdlib.h>

#include "loadpng.h"

//-------------------------------------------------------------------------

bool
loadPng(
    const char *file,
    IMAGE_T *image)
{
    FILE *fpin = fopen(file, "rb");

    if (fpin == NULL)
    {
        perror("Error: cannot open file");
        return false;
    }

    //---------------------------------------------------------------------

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 NULL,
                                                 NULL);

    if (png_ptr == NULL)
    {
        exit(EXIT_FAILURE);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr, 0, 0);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    png_init_io(png_ptr, fpin);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);

    png_byte colour_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    //---------------------------------------------------------------------

    void *buffer = malloc(width * height * 3);

    //---------------------------------------------------------------------

    double gamma = 0.0;

    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
    {
        png_set_gamma(png_ptr, 2.2, gamma);
    }

    //---------------------------------------------------------------------

    if (colour_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if ((colour_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
    {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    if (bit_depth == 16)
    {
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
        png_set_strip_16(png_ptr);
#else
        png_set_strip_16(png_ptr);
#endif
    }

    if (colour_type == PNG_COLOR_TYPE_GRAY ||
        colour_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png_ptr);
    }

    //---------------------------------------------------------------------

    png_color_16 *image_background = NULL;

    if (png_get_bKGD(png_ptr, info_ptr, &image_background))
    {
        png_set_background(png_ptr,
                           image_background,
                           PNG_BACKGROUND_GAMMA_FILE,
                           1,
                           1.0);
    }
    else
    {
        png_color_16 background = { 0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

        png_set_background(png_ptr,
                           &background,
                           PNG_BACKGROUND_GAMMA_SCREEN,
                           0,
                           1.0);
    }

    //---------------------------------------------------------------------

    png_read_update_info(png_ptr, info_ptr);

    //---------------------------------------------------------------------

    png_uint_32 row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    png_bytepp row_pointers = malloc(sizeof(png_bytep) * height);

    png_uint_32 j = 0;
    for (j = 0 ; j < height ; j++)
    {
        row_pointers[j] = buffer + (j * row_bytes);
    }

    //---------------------------------------------------------------------

    png_read_image(png_ptr, row_pointers);

    //---------------------------------------------------------------------

    fclose(fpin);

    free(row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    //---------------------------------------------------------------------

    bool result = initImage(image, width, height, true);

    if (result == false)
    {
        free(buffer);
        return false;
    }

    for (j = 0 ; j < height ; j++)
    {
        png_uint_32 i = 0;
        for (i = 0 ; i < width ; i++)
        {
            uint8_t *pixel = buffer + (i * 3) + (j * row_bytes);

            RGB8_T rgb = { pixel[0], pixel[1], pixel[2] };
            setPixelRGB(image, i, j, &rgb);        
        }
    }

    free(buffer);

    //---------------------------------------------------------------------

    return true;
}

