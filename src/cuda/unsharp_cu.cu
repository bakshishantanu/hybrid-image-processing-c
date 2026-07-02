// unsharp_cu.cu
#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda_runtime.h>

#define KERNEL_SIZE 5
#define SIGMA 1.5
#define UNSHARP_AMOUNT 1.5

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

__constant__ double d_unsharp_kernel[KERNEL_SIZE];

__global__ void UnsharpOneDBlurKernel(unsigned char *src, unsigned char *dst, int width, int height, int bytes_per_pixel, int is_vertical)
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
            int nx = x;
            int ny = y;
            if (is_vertical) ny = y + k;
            else nx = x + k;
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                int idx = (ny * width + nx) * bytes_per_pixel;
                double w = d_unsharp_kernel[k + half_size];
                for (int c = 0; c < bytes_per_pixel; c++)
                {
                    sum[c] += src[idx + c] * w;
                }
                weight_sum += w;
            }
        }

        int idx = (y * width + x) * bytes_per_pixel;
        for (int c = 0; c < bytes_per_pixel; c++)
        {
            dst[idx + c] = (unsigned char)(sum[c] / weight_sum);
        }
    }
}

__global__ void UnsharpMaskKernel(unsigned char *original, unsigned char *blurred, unsigned char *out, int data_size)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < data_size)
    {
        double val = original[i] + UNSHARP_AMOUNT * ((double)original[i] - (double)blurred[i]);
        if (val < 0.0) val = 0.0;
        if (val > 255.0) val = 255.0;
        out[i] = (unsigned char)(val + 0.5);
    }
}

extern "C" void BMP_UnsharpMask(BMP_Image *img)
{
    double h_kernel[KERNEL_SIZE];
    generateKernel(h_kernel, KERNEL_SIZE, SIGMA);
    cudaMemcpyToSymbol(d_unsharp_kernel, h_kernel, KERNEL_SIZE * sizeof(double));

    unsigned char *d_original, *d_temp, *d_blurred;
    cudaMalloc((void**)&d_original, img->data_size);
    cudaMalloc((void**)&d_temp, img->data_size);
    cudaMalloc((void**)&d_blurred, img->data_size);
    
    cudaMemcpy(d_original, img->data, img->data_size, cudaMemcpyHostToDevice);

    dim3 blockSize(16, 16);
    dim3 numBlocks((img->width + blockSize.x - 1) / blockSize.x, (img->height + blockSize.y - 1) / blockSize.y);

    UnsharpOneDBlurKernel<<<numBlocks, blockSize>>>(d_original, d_temp, img->width, img->height, img->bytes_per_pixel, 0);
    cudaDeviceSynchronize();
    UnsharpOneDBlurKernel<<<numBlocks, blockSize>>>(d_temp, d_blurred, img->width, img->height, img->bytes_per_pixel, 1);
    cudaDeviceSynchronize();

    int blockSize1D = 256;
    int numBlocks1D = (img->data_size + blockSize1D - 1) / blockSize1D;
    
    UnsharpMaskKernel<<<numBlocks1D, blockSize1D>>>(d_original, d_blurred, d_temp, img->data_size);
    cudaDeviceSynchronize();

    cudaMemcpy(img->data, d_temp, img->data_size, cudaMemcpyDeviceToHost);

    cudaFree(d_original);
    cudaFree(d_temp);
    cudaFree(d_blurred);
}
