// main_cu.cu
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cuda_runtime.h>

extern "C" {
    #include "bmpimage.h"
    #include "bmpfile.h"
    #include "bmpfunctions.h"
}

int main(int argc, char **argv)
{
    int csv_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) {
            csv_mode = 1;
            for (int j = i; j < argc - 1; j++) argv[j] = argv[j + 1];
            argc--;
            break;
        }
    }

    if (argc < 6) {
        printf("Usage: image <input> <gray> <blur> <sobel> <unsharp> [--csv]\n");
        return EXIT_FAILURE;
    }

    BMP_Image *img = BMP_Open(argv[1]);
    if (img == NULL) return EXIT_FAILURE;

    if (!csv_mode) {
        printf("Loaded %s: %ux%u (%u bpp), CUDA execution\n", argv[1], img->width, img->height, img->bytes_per_pixel * 8);
    }

    unsigned char *original_data = (unsigned char *)malloc(img->data_size);
    if (original_data) memcpy(original_data, img->data, img->data_size);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    float gray_ms=0, blur_ms=0, sobel_ms=0, unsharp_ms=0;

    cudaEventRecord(start);
    BMP_Gray(img);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&gray_ms, start, stop);
    if (!csv_mode) printf("Grayscale time: %.3f ms\n", gray_ms);

    if (BMP_Save(img, argv[2]) == 0) {
        BMP_Destroy(img); free(original_data); return EXIT_FAILURE;
    }

    cudaEventRecord(start);
    BMP_GaussianBlur(img);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&blur_ms, start, stop);
    if (!csv_mode) printf("Gaussian blur time: %.3f ms\n", blur_ms);

    if (BMP_Save(img, argv[3]) == 0) {
        BMP_Destroy(img); free(original_data); return EXIT_FAILURE;
    }

    cudaEventRecord(start);
    BMP_Sobel(img);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&sobel_ms, start, stop);
    if (!csv_mode) printf("Sobel filter time: %.3f ms\n", sobel_ms);

    if (BMP_Save(img, argv[4]) == 0) {
        BMP_Destroy(img); free(original_data); return EXIT_FAILURE;
    }

    if (original_data) {
        memcpy(img->data, original_data, img->data_size);
        free(original_data);
        original_data = NULL;
    }

    cudaEventRecord(start);
    BMP_UnsharpMask(img);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&unsharp_ms, start, stop);
    if (!csv_mode) printf("Unsharp mask time: %.3f ms\n", unsharp_ms);

    if (BMP_Save(img, argv[5]) == 0) {
        BMP_Destroy(img); return EXIT_FAILURE;
    }

    float total_ms = gray_ms + blur_ms + sobel_ms + unsharp_ms;

    if (csv_mode) {
        printf("cuda,%s,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,1,1\n",
               argv[1], img->width, img->height,
               img->width * img->height,
               gray_ms, blur_ms, sobel_ms, unsharp_ms, total_ms);
    } else {
        printf("Total time: %.3f ms\n", total_ms);
    }

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    BMP_Destroy(img);
    return EXIT_SUCCESS;
}
