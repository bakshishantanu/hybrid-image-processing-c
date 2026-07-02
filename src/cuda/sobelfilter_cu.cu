// sobelfilter_cu.cu
#include "bmpfunctions.h"
#include <stdio.h>
#include <math.h>
#include <cuda_runtime.h>

__constant__ int d_SOBEL_X[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

__constant__ int d_SOBEL_Y[3][3] = {
    {-1, -2, -1},
    {0,  0,  0},
    {1,  2,  1}
};

__global__ void applySobelKernel(unsigned char *src, int *gradient, int width, int height, int bytes_per_pixel, int is_vertical)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x > 0 && x < width - 1 && y > 0 && y < height - 1)
    {
        int sum = 0;            
        for (int ky = -1; ky <= 1; ky++)
        {
            for (int kx = -1; kx <= 1; kx++)
            {
                int pixel_index = ((y + ky) * width + (x + kx)) * bytes_per_pixel;
                int intensity = src[pixel_index];

                if (is_vertical)
                    sum += intensity * d_SOBEL_Y[ky + 1][kx + 1];
                else
                    sum += intensity * d_SOBEL_X[ky + 1][kx + 1];
            }
        }
        gradient[y * width + x] = sum;
    }
}

__global__ void computeGradientKernel(int *grad_x, int *grad_y, unsigned char *dst, int width, int height, int bytes_per_pixel)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height)
    {
        int index = (y * width + x);
        int gx = grad_x[index];
        int gy = grad_y[index];
        int magnitude = (int)sqrt((double)(gx * gx + gy * gy));
        magnitude = (magnitude > 255) ? 255 : magnitude;

        for (int c = 0; c < bytes_per_pixel; c++)
        {
            dst[index * bytes_per_pixel + c] = (unsigned char)magnitude;
        }            
    }
}

extern "C" void BMP_Sobel(BMP_Image *img)
{
    int width = img->width;
    int height = img->height;
    int data_size = img->data_size;

    unsigned char *d_src;
    int *d_grad_x, *d_grad_y;

    cudaMalloc((void**)&d_src, data_size);
    cudaMalloc((void**)&d_grad_x, width * height * sizeof(int));
    cudaMalloc((void**)&d_grad_y, width * height * sizeof(int));

    cudaMemcpy(d_src, img->data, data_size, cudaMemcpyHostToDevice);

    dim3 blockSize(16, 16);
    dim3 numBlocks((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    applySobelKernel<<<numBlocks, blockSize>>>(d_src, d_grad_x, width, height, img->bytes_per_pixel, 0);
    applySobelKernel<<<numBlocks, blockSize>>>(d_src, d_grad_y, width, height, img->bytes_per_pixel, 1);
    cudaDeviceSynchronize();

    computeGradientKernel<<<numBlocks, blockSize>>>(d_grad_x, d_grad_y, d_src, width, height, img->bytes_per_pixel);
    cudaDeviceSynchronize();

    cudaMemcpy(img->data, d_src, data_size, cudaMemcpyDeviceToHost);

    cudaFree(d_src);
    cudaFree(d_grad_x);
    cudaFree(d_grad_y);
}
