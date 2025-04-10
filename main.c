#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bmpimage.h"
#include "bmpfile.h"
#include "bmpfunctions.h"

int main(int argc, char **argv)
{
    if (argc < 5) 
    {
        // Usage on cmd: image input.bmp output.bmp
        printf("Usage: image <input> <gray> <blur> <sobel>\n");
        return EXIT_FAILURE;
    }

    BMP_Image *img = BMP_Open(argv[1]);
    if (img == NULL) 
    {
        return EXIT_FAILURE;
    }

    clock_t start_gray = clock();
    BMP_Gray(img);
    clock_t end_gray = clock();
    printf("Grayscale time: %.3f ms\n", 1000.0 * (end_gray - start_gray) / CLOCKS_PER_SEC);

    if (BMP_Save(img, argv[2]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    clock_t start_blur = clock();
    BMP_GaussianBlur(img);
    clock_t end_blur = clock();
    printf("Gaussian blur time: %.3f ms\n", 1000.0 * (end_blur - start_blur) / CLOCKS_PER_SEC);

    if (BMP_Save(img, argv[3]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    clock_t start_sobel = clock();
    BMP_Sobel(img);
    clock_t end_sobel = clock();
    printf("Sobel filter time: %.3f ms\n", 1000.0 * (end_sobel - start_sobel) / CLOCKS_PER_SEC);

    if (BMP_Save(img, argv[4]) == 0)
    {
        printf("Output file invalid!\n");
        BMP_Destroy(img);
        return EXIT_FAILURE;
    }

    BMP_Destroy(img);
    return EXIT_SUCCESS;
}