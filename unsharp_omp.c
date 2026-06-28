#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define KERNEL_SIZE 5
#define SIGMA 1.5
#define UNSHARP_AMOUNT 1.5

static void generateKernel(double *kernel, int size, double sigma)
{
    double sum = 0.0;
    int half_size = size / 2;

    for (int i = 0; i < size; i++)
    {
        double x = i - half_size;
        kernel[i] = exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * M_PI) * sigma);
        sum += kernel[i];
    }

    for (int i = 0; i < size; i++)
    {
        kernel[i] /= sum;
    }
}

static void OneDBlur(const unsigned char *src, unsigned char *dst, int width, int height, int bpp, const double *kernel, int size, int is_vertical)
{
    int half_size = size / 2;

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            double sum[3] = {0.0, 0.0, 0.0};
            double weight_sum = 0.0;

            for (int k = -half_size; k <= half_size; k++)
            {
                int nx = x, ny = y;
                if (is_vertical)
                    ny = y + k;
                else
                    nx = x + k;

                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                {
                    int idx = (ny * width + nx) * bpp;
                    double w = kernel[k + half_size];
                    for (int c = 0; c < bpp; c++)
                    {
                        sum[c] += src[idx + c] * w;
                    }
                    weight_sum += w;
                }
            }

            int idx = (y * width + x) * bpp;
            for (int c = 0; c < bpp; c++)
            {
                dst[idx + c] = (unsigned char)(sum[c] / weight_sum);
            }
        }
    }
}

void BMP_UnsharpMask(BMP_Image *img)
{
    unsigned int data_size = img->data_size;
    int width = img->width;
    int height = img->height;
    int bpp = img->bytes_per_pixel;

    unsigned char *original = (unsigned char *)malloc(data_size);
    unsigned char *temp = (unsigned char *)malloc(data_size);

    if (!original || !temp)
    {
        printf("Memory allocation failed!\n");
        free(original);
        free(temp);
        return;
    }

    /* Save original before blurring */
    memcpy(original, img->data, data_size);

    /* Apply separable Gaussian blur to img->data */
    double kernel[KERNEL_SIZE];
    generateKernel(kernel, KERNEL_SIZE, SIGMA);

    OneDBlur(img->data, temp, width, height, bpp, kernel, KERNEL_SIZE, 0);
    memcpy(img->data, temp, data_size);

    OneDBlur(img->data, temp, width, height, bpp, kernel, KERNEL_SIZE, 1);
    memcpy(img->data, temp, data_size);

    /* Unsharp mask: output = clamp(original + amount * (original - blurred), 0, 255) */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)data_size; i++)
    {
        double val = original[i] + UNSHARP_AMOUNT * ((double)original[i] - (double)img->data[i]);
        if (val < 0.0) val = 0.0;
        if (val > 255.0) val = 255.0;
        img->data[i] = (unsigned char)(val + 0.5);
    }

    free(original);
    free(temp);
}
