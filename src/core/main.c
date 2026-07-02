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
    /* Check for --csv flag */
    int csv_mode = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--csv") == 0)
        {
            csv_mode = 1;
            for (int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argc--;
            break;
        }
    }

    if (argc < 6) 
    {
        printf("Usage: image <input> <gray> <blur> <sobel> <unsharp> [--csv]\n");
        return EXIT_FAILURE;
    }

    BMP_Image *img = BMP_Open(argv[1]);
    if (img == NULL) 
    {
        return EXIT_FAILURE;
    }

    int threads = 1;
    #ifdef _OPENMP
    threads = omp_get_max_threads();
    #endif

    if (!csv_mode)
    {
        printf("Loaded %s: %ux%u (%u bpp)", argv[1], img->width, img->height, img->bytes_per_pixel * 8);
        #ifdef _OPENMP
        printf(", %d OpenMP threads", threads);
        #endif
        printf("\n");
    }

    /* Save original image data for unsharp mask (Sobel destroys it) */
    unsigned char *original_data = (unsigned char *)malloc(img->data_size);
    if (original_data)
    {
        memcpy(original_data, img->data, img->data_size);
    }

    TIMER_TYPE start, end;
    double gray_ms, blur_ms, sobel_ms, unsharp_ms;

    /* Grayscale */
    start = TIMER_NOW();
    BMP_Gray(img);
    end = TIMER_NOW();
    gray_ms = TIMER_MS(start, end);
    if (!csv_mode) printf("Grayscale time: %.3f ms\n", gray_ms);

    if (BMP_Save(img, argv[2]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        free(original_data);
        return EXIT_FAILURE;
    }

    /* Gaussian Blur */
    start = TIMER_NOW();
    BMP_GaussianBlur(img);
    end = TIMER_NOW();
    blur_ms = TIMER_MS(start, end);
    if (!csv_mode) printf("Gaussian blur time: %.3f ms\n", blur_ms);

    if (BMP_Save(img, argv[3]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        free(original_data);
        return EXIT_FAILURE;
    }

    /* Sobel */
    start = TIMER_NOW();
    BMP_Sobel(img);
    end = TIMER_NOW();
    sobel_ms = TIMER_MS(start, end);
    if (!csv_mode) printf("Sobel filter time: %.3f ms\n", sobel_ms);

    if (BMP_Save(img, argv[4]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        free(original_data);
        return EXIT_FAILURE;
    }

    /* Restore original image for unsharp mask */
    if (original_data)
    {
        memcpy(img->data, original_data, img->data_size);
        free(original_data);
        original_data = NULL;
    }

    /* Unsharp Mask */
    start = TIMER_NOW();
    BMP_UnsharpMask(img);
    end = TIMER_NOW();
    unsharp_ms = TIMER_MS(start, end);
    if (!csv_mode) printf("Unsharp mask time: %.3f ms\n", unsharp_ms);

    if (BMP_Save(img, argv[5]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    double total_ms = gray_ms + blur_ms + sobel_ms + unsharp_ms;

    if (csv_mode)
    {
        #ifdef _OPENMP
        const char *model = "openmp";
        #else
        const char *model = "sequential";
        #endif
        printf("%s,%s,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%d,1\n",
               model, argv[1], img->width, img->height,
               img->width * img->height,
               gray_ms, blur_ms, sobel_ms, unsharp_ms, total_ms,
               threads);
    }
    else
    {
        printf("Total time: %.3f ms\n", total_ms);
    }

    BMP_Destroy(img);
    return EXIT_SUCCESS;
}