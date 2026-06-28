#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "mpi.h"

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

static void OneDLocalBlur(unsigned char *src, unsigned char *dest, int width, int height, int bytes_per_pixel, const double *kernel, int size, int is_vertical)
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

void BMP_UnsharpMask_Hybrid(BMP_Image *img, int rank, int size)
{
    int width = img->width;
    int height = img->height;
    int bpp = img->bytes_per_pixel;
    int row_size = width * bpp;
    int half = KERNEL_SIZE / 2;

    int rows_per_proc = height / size;
    int remainder = height % size;
    int local_height = rows_per_proc + (rank < remainder ? 1 : 0);
    int local_size = local_height * row_size;

    int padded_height = local_height + 2 * half;
    int padded_size = padded_height * row_size;

    unsigned char *local_data = (unsigned char *)malloc(local_size);
    unsigned char *local_original = (unsigned char *)malloc(local_size);
    unsigned char *local_hblur = (unsigned char *)malloc(local_size);
    unsigned char *local_padded = (unsigned char *)calloc(padded_size, 1);
    unsigned char *temp_vblur = (unsigned char *)malloc(padded_size);
    unsigned char *local_vblur = (unsigned char *)malloc(local_size);
    double kernel[KERNEL_SIZE];

    if (!local_data || !local_original || !local_hblur || !local_padded || !temp_vblur || !local_vblur)
    {
        fprintf(stderr, "Error: Memory allocation failed in UnsharpMask_Hybrid\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int *sendcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));

    if (!sendcounts || !displs)
    {
        fprintf(stderr, "Error: Memory allocation failed for scatter arrays\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int offset = 0;
    for (int i = 0; i < size; i++)
    {
        int rows = rows_per_proc + (i < remainder ? 1 : 0);
        sendcounts[i] = rows * row_size;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    MPI_Scatterv(img->data, sendcounts, displs, MPI_UNSIGNED_CHAR,
                 local_data, local_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    /* Save original local data before blurring */
    memcpy(local_original, local_data, local_size);

    generateKernel(kernel, KERNEL_SIZE, SIGMA);

    /* Horizontal blur — no ghost rows needed */
    OneDLocalBlur(local_data, local_hblur, width, local_height, bpp, kernel, KERNEL_SIZE, 0);

    /* Build padded buffer for vertical blur */
    memcpy(local_padded + half * row_size, local_hblur, local_size);

    /* Exchange ghost rows with neighbors */
    if (rank > 0)
    {
        MPI_Sendrecv(local_hblur, half * row_size, MPI_UNSIGNED_CHAR, rank - 1, 0,
                     local_padded, half * row_size, MPI_UNSIGNED_CHAR, rank - 1, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank < size - 1)
    {
        MPI_Sendrecv(local_hblur + (local_height - half) * row_size, half * row_size,
                     MPI_UNSIGNED_CHAR, rank + 1, 1,
                     local_padded + (local_height + half) * row_size, half * row_size,
                     MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    /* Vertical blur on full padded buffer */
    OneDLocalBlur(local_padded, temp_vblur, width, padded_height, bpp, kernel, KERNEL_SIZE, 1);

    /* Extract center rows = blurred result */
    memcpy(local_vblur, temp_vblur + half * row_size, local_size);

    /* Apply unsharp mask: output = clamp(original + amount * (original - blurred)) */
    int pixel;
    #pragma omp parallel for schedule(static)
    for (pixel = 0; pixel < local_size; pixel++)
    {
        double val = local_original[pixel] + UNSHARP_AMOUNT * ((double)local_original[pixel] - (double)local_vblur[pixel]);
        if (val < 0.0) val = 0.0;
        if (val > 255.0) val = 255.0;
        local_data[pixel] = (unsigned char)(val + 0.5);
    }

    MPI_Gatherv(local_data, local_size, MPI_UNSIGNED_CHAR,
                img->data, sendcounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(local_data);
    free(local_original);
    free(local_hblur);
    free(local_padded);
    free(temp_vblur);
    free(local_vblur);
    free(sendcounts);
    free(displs);
}
