#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "mpi.h"
#include "bmpimage.h"
#include "bmpfunctions.h"

// Sobel Kernels
const int SOBEL_X[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

const int SOBEL_Y[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

void applyLocalSobel(unsigned char *data, int *gradient, int width, int height, int bpp, int is_vertical)
{
    int y;
    #pragma omp parallel for schedule(static)
    for (y = 1; y < height - 1; y++)
    {
        int x;
        for (x = 1; x < width - 1; x++)
        {
            int sum = 0;
            int ky, kx;
            for (ky = -1; ky <= 1; ky++)
            {
                for (kx = -1; kx <= 1; kx++)
                {
                    int idx = ((y + ky) * width + (x + kx)) * bpp;
                    int intensity = data[idx];
                    sum += intensity * (is_vertical ? SOBEL_Y[ky + 1][kx + 1] : SOBEL_X[ky + 1][kx + 1]);
                }
            }
            gradient[y * width + x] = sum;
        }
    }
}

void computeLocalGradient(int *grad_x, int *grad_y, unsigned char *dst, int width, int height, int bpp)
{
    int y;
    #pragma omp parallel for schedule(static)
    for (y = 0; y < height; y++)
    {
        int x;
        for (x = 0; x < width; x++)
        {
            int index = y * width + x;
            int mag = (int)sqrt((double)(grad_x[index] * grad_x[index] + grad_y[index] * grad_y[index]));
            if (mag > 255) mag = 255;

            int c;
            for (c = 0; c < bpp; c++)
            {
                dst[index * bpp + c] = (unsigned char)mag;
            }
        }
    }
}

void BMP_Sobel_Hybrid(BMP_Image *img, int rank, int size)
{
    int width = img->width;
    int height = img->height;
    int bpp = img->bytes_per_pixel;

    int rows_per_proc = height / size;
    int remainder = height % size;

    int local_height = rows_per_proc + (rank < remainder ? 1 : 0);
    int start_row = rank * rows_per_proc + (rank < remainder ? rank : remainder);

    int local_size = local_height * width * bpp;
    int padded_height = local_height + 2;
    int padded_size = padded_height * width * bpp;

    unsigned char *local_data = (unsigned char *)malloc(local_size);
    unsigned char *padded_data = (unsigned char *)calloc(padded_size, 1);
    unsigned char *local_result = (unsigned char *)malloc(local_size);
    int *grad_x = (int *)malloc(padded_height * width * sizeof(int));
    int *grad_y = (int *)malloc(padded_height * width * sizeof(int));

    int *sendcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));
    int offset = 0;
    int i;
    for (i = 0; i < size; i++)
    {
        int rows = rows_per_proc + (i < remainder ? 1 : 0);
        sendcounts[i] = rows * width * bpp;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    MPI_Scatterv(img->data, sendcounts, displs, MPI_UNSIGNED_CHAR,
                 local_data, local_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    memcpy(padded_data + (width * bpp), local_data, local_size);

    if (rank > 0)
    {
        MPI_Sendrecv(local_data, width * bpp, MPI_UNSIGNED_CHAR, rank - 1, 0,
                     padded_data, width * bpp, MPI_UNSIGNED_CHAR, rank - 1, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank < size - 1)
    {
        MPI_Sendrecv(local_data + (local_height - 1) * width * bpp, width * bpp, MPI_UNSIGNED_CHAR,
                     rank + 1, 1, padded_data + (local_height + 1) * width * bpp, width * bpp,
                     MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    applyLocalSobel(padded_data, grad_x, width, padded_height, bpp, 0);
    applyLocalSobel(padded_data, grad_y, width, padded_height, bpp, 1);
    computeLocalGradient(grad_x + width, grad_y + width, local_result, width, local_height, bpp);

    MPI_Gatherv(local_result, local_size, MPI_UNSIGNED_CHAR,
                img->data, sendcounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(local_data);
    free(padded_data);
    free(local_result);
    free(grad_x);
    free(grad_y);
    free(sendcounts);
    free(displs);
}