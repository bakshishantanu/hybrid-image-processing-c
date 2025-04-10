#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define KERNEL_SIZE 5
#define SIGMA 1.5

void generateKernel(double *kernel, int size, double sigma)
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

void OneDBlur(BMP_Image *img, unsigned char *temp_data, const double *kernel, int size, int is_vertical)
{
    int width = img->width;
    int height = img->height;
    int half_size = size / 2;
    int bytes_per_pixel = img->bytes_per_pixel;

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            double sum[3] = {0.0, 0.0, 0.0};
            double weight_sum = 0.0;

            for (int k = -half_size; k <= half_size; k++)
            {
                int new_x = x, new_y = y;
                if (is_vertical)
                    new_y = y + k;
                else
                    new_x = x + k;

                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height)
                {
                    int index = (new_y * width + new_x) * bytes_per_pixel;
                    double weight = kernel[k + half_size];

                    for (int c = 0; c < bytes_per_pixel; c++)
                    {
                        sum[c] += img->data[index + c] * weight;
                    }
                    weight_sum += weight;
                }
            }

            int index = (y * width + x) * bytes_per_pixel;
            for (int c = 0; c < bytes_per_pixel; c++)
            {
                temp_data[index + c] = (unsigned char)(sum[c] / weight_sum);
            }
        }
    }
}

void BMP_GaussianBlur(BMP_Image *img)
{
    double kernel[KERNEL_SIZE];
    generateKernel(kernel, KERNEL_SIZE, SIGMA);

    unsigned char *temp_data = (unsigned char *)malloc(img->data_size);
    if (!temp_data)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    OneDBlur(img, temp_data, kernel, KERNEL_SIZE, 0);
    memcpy(img->data, temp_data, img->data_size);

    OneDBlur(img, temp_data, kernel, KERNEL_SIZE, 1);
    memcpy(img->data, temp_data, img->data_size);

    free(temp_data);
}