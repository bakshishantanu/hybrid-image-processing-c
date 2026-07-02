// bmpgray_cu.cu
#include "bmpfunctions.h"
#include <stdio.h>
#include <cuda_runtime.h>

__global__ void RGB2GrayKernel(unsigned char *data, int data_size)
{
    int pixel = (blockIdx.x * blockDim.x + threadIdx.x) * 3;
    if (pixel < data_size)
    {
        unsigned char blue  = data[pixel];
        unsigned char green = data[pixel+1];
        unsigned char red   = data[pixel+2];
        
        double gray_d = 0.299 * red + 0.587 * green + 0.114 * blue;
        unsigned char gray = (unsigned char)(gray_d + 0.5);
        
        data[pixel] = gray;
        data[pixel+1] = gray;
        data[pixel+2] = gray;
    }
}

extern "C" void BMP_Gray(BMP_Image *img)
{
    unsigned char *d_data;
    cudaMalloc((void**)&d_data, img->data_size);
    cudaMemcpy(d_data, img->data, img->data_size, cudaMemcpyHostToDevice);
    
    int num_pixels = img->data_size / 3;
    int blockSize = 256;
    int numBlocks = (num_pixels + blockSize - 1) / blockSize;
    
    RGB2GrayKernel<<<numBlocks, blockSize>>>(d_data, img->data_size);
    cudaDeviceSynchronize();
    
    cudaMemcpy(img->data, d_data, img->data_size, cudaMemcpyDeviceToHost);
    cudaFree(d_data);
}
