#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "mpi.h"

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

void OneDLocalBlur(unsigned char *src, unsigned char *dest, int width, int height, int bytes_per_pixel, double *kernel, int size, int is_vertical)
{
    int half_size = size / 2;
    int y;

    #pragma omp parallel for
    for (y = 0; y < height; y++)
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

                    for (int c = 0; c < bytes_per_pixel; c++)
                    {
                        sum[c] += src[index + c] * kernel[k + half_size];
                    }
                    weight_sum += kernel[k + half_size];
                }
            }

            int index = (y * width + x) * bytes_per_pixel;
            for (int c = 0; c < bytes_per_pixel; c++)
            {
                dest[index + c] = (unsigned char)(sum[c] / weight_sum);
            }
        }
    }
}

void BMP_GaussianBlur_Hybrid(BMP_Image *img, int rank, int size)
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
    unsigned char *local_hblur = (unsigned char *)malloc(local_size);
    unsigned char *local_padded = (unsigned char *)calloc(padded_size, 1);
    unsigned char *local_vblur = (unsigned char *)malloc(local_size);
    double kernel[KERNEL_SIZE];

    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    int offset = 0;
    for (int i = 0; i < size; i++)
    {
        int rows = rows_per_proc + (i < remainder ? 1 : 0);
        sendcounts[i] = rows * width * bpp;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    MPI_Scatterv(img->data, sendcounts, displs, MPI_UNSIGNED_CHAR, local_data, local_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    generateKernel(kernel, KERNEL_SIZE, SIGMA);
    OneDLocalBlur(local_data, local_hblur, width, local_height, bpp, kernel, KERNEL_SIZE, 0);
    memcpy(local_padded + (width * bpp), local_hblur, local_size);

    if (rank > 0)
    {
        MPI_Sendrecv(local_hblur, width * bpp, MPI_UNSIGNED_CHAR, rank - 1, 0, local_padded, width * bpp,
                     MPI_UNSIGNED_CHAR, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank < size - 1)
    {
        MPI_Sendrecv(local_hblur + (local_height - 1) * width * bpp, width * bpp,
                     MPI_UNSIGNED_CHAR, rank + 1, 1, local_padded + (local_height + 1) * width * bpp, width * bpp,
                     MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    OneDLocalBlur(local_padded, local_vblur, width, local_height, bpp, kernel, KERNEL_SIZE, 1);
    MPI_Gatherv(local_vblur, local_size, MPI_UNSIGNED_CHAR, img->data, sendcounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(local_data);
    free(local_hblur);
    free(local_padded);
    free(local_vblur);
    free(sendcounts);
    free(displs);
}