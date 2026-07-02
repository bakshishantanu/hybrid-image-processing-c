// bmpblur_cu.cu
#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda_runtime.h>

#define KERNEL_SIZE 5
#define SIGMA 1.5

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void generateKernel(double *kernel, int size, double sigma)
{
    double sum = 0.0;
    int half_size = size / 2;
    for (int i = 0; i < size; i++) {
        double x = i - half_size;
        kernel[i] = exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * M_PI) * sigma);
        sum += kernel[i];
    }
    for (int i = 0; i < size; i++) {
        kernel[i] /= sum;
    }
}

__constant__ double d_kernel[KERNEL_SIZE];

__global__ void OneDBlurKernel(unsigned char *src, unsigned char *dst, int width, int height, int bytes_per_pixel, int is_vertical)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int half_size = KERNEL_SIZE / 2;

    if (x < width && y < height)
    {
        double sum[3] = {0.0, 0.0, 0.0};
        double weight_sum = 0.0;

        for (int k = -half_size; k <= half_size; k++)
        {
            int new_x = x;
            int new_y = y;
            if (is_vertical) new_y = y + k;
            else new_x = x + k;
            
            if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height)
            {
                int index = (new_y * width + new_x) * bytes_per_pixel;
                double w = d_kernel[k + half_size];
                for (int c = 0; c < bytes_per_pixel; c++)
                {
                    sum[c] += src[index + c] * w;
                }
                weight_sum += w;
            }
        }

        int index = (y * width + x) * bytes_per_pixel;
        for (int c = 0; c < bytes_per_pixel; c++)
        {
            dst[index + c] = (unsigned char)(sum[c] / weight_sum);
        }
    }
}

extern "C" void BMP_GaussianBlur(BMP_Image *img)
{
    double h_kernel[KERNEL_SIZE];
    generateKernel(h_kernel, KERNEL_SIZE, SIGMA);
    cudaMemcpyToSymbol(d_kernel, h_kernel, KERNEL_SIZE * sizeof(double));

    unsigned char *d_src, *d_dst;
    cudaMalloc((void**)&d_src, img->data_size);
    cudaMalloc((void**)&d_dst, img->data_size);
    cudaMemcpy(d_src, img->data, img->data_size, cudaMemcpyHostToDevice);

    dim3 blockSize(16, 16);
    dim3 numBlocks((img->width + blockSize.x - 1) / blockSize.x, (img->height + blockSize.y - 1) / blockSize.y);

    OneDBlurKernel<<<numBlocks, blockSize>>>(d_src, d_dst, img->width, img->height, img->bytes_per_pixel, 0);
    cudaDeviceSynchronize();
    OneDBlurKernel<<<numBlocks, blockSize>>>(d_dst, d_src, img->width, img->height, img->bytes_per_pixel, 1);
    cudaDeviceSynchronize();

    cudaMemcpy(img->data, d_src, img->data_size, cudaMemcpyDeviceToHost);
    cudaFree(d_src);
    cudaFree(d_dst);
}
