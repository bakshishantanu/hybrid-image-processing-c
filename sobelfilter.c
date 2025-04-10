#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bmpimage.h"
#include "bmpfunctions.h"

// Sobel Kernels
int SOBEL_X[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int SOBEL_Y[3][3] = {
    {-1, -2, -1},
    {0,  0,  0},
    {1,  2,  1}
};

void applySobel(BMP_Image *img, int *gradient, int is_vertical)
{
    int width = img->width;
    int height = img->height;
    int bytes_per_pixel = img->bytes_per_pixel;
    
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sum = 0;            
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int pixel_index = ((y + ky) * width + (x + kx)) * bytes_per_pixel;
                    int intensity = img->data[pixel_index];

                    if (is_vertical)
                        sum += intensity * SOBEL_Y[ky + 1][kx + 1];
                    else
                        sum += intensity * SOBEL_X[ky + 1][kx + 1];
                }
            }
            gradient[y * width + x] = sum;
        }
    }
}

void computeGradient(int *grad_x, int *grad_y, BMP_Image *img)
{
    int width = img->width;
    int height = img->height;
    int bytes_per_pixel = img->bytes_per_pixel;
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x);
            int magnitude = (int)sqrt(grad_x[index] * grad_x[index] + grad_y[index] * grad_y[index]);
            magnitude = (magnitude > 255) ? 255 : magnitude;

            for (int c = 0; c < bytes_per_pixel; c++)
            {
                img->data[index * bytes_per_pixel + c] = (unsigned char)magnitude;
            }            
        }
    }
}

void BMP_Sobel(BMP_Image *img)
{
    int width = img->width;
    int height = img->height;
    int *grad_x = (int *)malloc(width * height * sizeof(int));
    int *grad_y = (int *)malloc(width * height * sizeof(int));

    if (!grad_x || !grad_y)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    applySobel(img, grad_x, 0);
    applySobel(img, grad_y, 1);
    computeGradient(grad_x, grad_y, img);
    free(grad_x);
    free(grad_y);
}