#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>

#include "image.h"
#include "lcd.h"

//-------------------------------------------------------------------------

void
triangle(
    LCD_T *lcd)
{
    int16_t xoffset = (lcd->width - 128) / 2;
    int16_t yoffset = (lcd->height - 128) / 2;

    int16_t y;
    for (y = 0 ; y < 128 ; y++)
    {
        int16_t x;
        for (x = 0 ; x < 128 ; x++)
        {
            int16_t r = 127 - x - (y / 2);
            int16_t g = x - (y / 2);
            int16_t b = 127 - r - g;

            if ((r >= 0) && (g >= 0))
            {
                r = (r * 255) / 127;
                g = (g * 255) / 127;
                b = (b * 255) / 127;

                setPixelLcd(lcd, x + xoffset, y + yoffset, packRGB(r, g, b));
            }
        }
    }
}

//-------------------------------------------------------------------------

void
triangleImage(
    LCD_T *lcd,
    bool dither,
    int16_t xoffset,
    int16_t yoffset)
{
    IMAGE_T image;
    initImage(&image, 128, 128, dither);

    int16_t y;
    for (y = 0 ; y < 128 ; y++)
    {
        int16_t x;
        for (x = 0 ; x < 128 ; x++)
        {
            int16_t r = 127 - x - (y / 2);
            int16_t g = x - (y / 2);
            int16_t b = 127 - r - g;

            if ((r >= 0) && (g >= 0))
            {
                r = (r * 255) / 127;
                g = (g * 255) / 127;
                b = (b * 255) / 127;

                RGB8_T rgb = { r, g, b };
                setPixelRGB(&image, x, y, &rgb);
            }
        }
    }

    putImageLcd(lcd, xoffset, yoffset, &image);

    destroyImage(&image);
}

//-------------------------------------------------------------------------

void test(
    bool rotate)
{
    LCD_T lcd;

    if (initLcd(&lcd, rotate) == false)
    {
        fprintf(stderr, "LCD initialization failed\n");
        exit(EXIT_FAILURE);
    }

    struct timeval start_time;
    struct timeval end_time;
    struct timeval diff;

	printf("testing LCD %dx%d\n", lcd.width, lcd.height);

    // red
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(255, 0, 0));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // yellow
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(255, 255, 0));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // green
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(0, 255, 0));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // cyan
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(0, 255, 255));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // blue
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(0, 0, 255));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // magenta
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(255, 0, 255));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // black
    gettimeofday(&start_time, NULL);
    clearLcd(&lcd, packRGB(0, 0, 0));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    clearLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);
    sleep(1);

    // dark grey
    gettimeofday(&start_time, NULL);
    filledBoxLcd(&lcd, 0, 0, lcd.width, lcd.height, packRGB(8, 8, 8));
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    filledBoxLcd took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);

    // black box
    filledBoxLcd(&lcd,
                 (lcd.width - 148) / 2,
                 (lcd.height - 148) / 2,
                 148,
                 148,
                 packRGB(0, 0, 0));

    gettimeofday(&start_time, NULL);
    triangle(&lcd);
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    triangle took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);

    sleep(1);

    // black box
    filledBoxLcd(&lcd,
                 (lcd.width - 148) / 2,
                 (lcd.height - 148) / 2,
                 148,
                 148,
                 packRGB(0, 0, 0));

    int16_t xoffset = (lcd.width - 128) / 2;
    int16_t yoffset = (lcd.height - 128) / 2;

    gettimeofday(&start_time, NULL);
    triangleImage(&lcd, false, xoffset, yoffset);
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    triangleImage took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);

    sleep(1);

    gettimeofday(&start_time, NULL);
    triangleImage(&lcd, true, xoffset, yoffset);
    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &diff);
    printf("    triangleImage (dithered) took - %d.%06d seconds\n",
           (int)diff.tv_sec,
           (int)diff.tv_usec);

    sleep(1);
	printf("\n");

    closeLcd(&lcd);
}

//-------------------------------------------------------------------------

void pwm()
{
    LCD_T lcd;

    if (initLcd(&lcd, true) == false)
    {
        fprintf(stderr, "LCD initialization failed\n");
        exit(EXIT_FAILURE);
    }

    int16_t xoffset = (lcd.width - 128) / 2;
    int16_t yoffset = (lcd.height - 128) / 2;

    triangleImage(&lcd, false, xoffset, yoffset);

    printf("pwm ... backlight\n");
    uint32_t backlight = 0;
    for (backlight = 0 ; backlight < 1024 ; backlight++)
    {
        backlightLcd(backlight);
        usleep(1000);
    }

    closeLcd(&lcd);
}

//-------------------------------------------------------------------------

int
main(void)
{
    if (getuid() != 0)
    {
        fprintf(stderr, "you must be root to run this program\n");
        exit(EXIT_FAILURE);
    }

    pwm();
	test(true);
	test(false);

    return 0 ;
}

