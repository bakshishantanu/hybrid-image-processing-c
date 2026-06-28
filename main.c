#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmpimage.h"
#include "bmpfile.h"
#include "bmpfunctions.h"

/*
 * High-resolution wall-clock timer.
 * When compiled with -fopenmp, uses omp_get_wtime() (nanosecond resolution).
 * Otherwise falls back to clock() (1ms resolution on Windows ucrt).
 */
#ifdef _OPENMP
    #include <omp.h>
    #define TIMER_TYPE    double
    #define TIMER_NOW()   omp_get_wtime()
    #define TIMER_MS(s,e) (((e) - (s)) * 1000.0)
#else
    #include <time.h>
    #define TIMER_TYPE    clock_t
    #define TIMER_NOW()   clock()
    #define TIMER_MS(s,e) (1000.0 * ((e) - (s)) / CLOCKS_PER_SEC)
#endif

int main(int argc, char **argv)
{
    if (argc < 5) 
    {
        printf("Usage: image <input> <gray> <blur> <sobel>\n");
        return EXIT_FAILURE;
    }

    BMP_Image *img = BMP_Open(argv[1]);
    if (img == NULL) 
    {
        return EXIT_FAILURE;
    }

    printf("Loaded %s: %ux%u (%u bpp)\n", argv[1], img->width, img->height, img->bytes_per_pixel * 8);

    TIMER_TYPE start, end;

    start = TIMER_NOW();
    BMP_Gray(img);
    end = TIMER_NOW();
    printf("Grayscale time: %.3f ms\n", TIMER_MS(start, end));

    if (BMP_Save(img, argv[2]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    start = TIMER_NOW();
    BMP_GaussianBlur(img);
    end = TIMER_NOW();
    printf("Gaussian blur time: %.3f ms\n", TIMER_MS(start, end));

    if (BMP_Save(img, argv[3]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    start = TIMER_NOW();
    BMP_Sobel(img);
    end = TIMER_NOW();
    printf("Sobel filter time: %.3f ms\n", TIMER_MS(start, end));

    if (BMP_Save(img, argv[4]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    BMP_Destroy(img);
    return EXIT_SUCCESS;
}