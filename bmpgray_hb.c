#include "bmpfunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "mpi.h"

static int RGB2Gray(unsigned char red, unsigned char green, unsigned char blue)
{
    double gray = 0.299 * red + 0.587 * green + 0.114 * blue;
    return (int)(gray + 0.5);
}

void BMP_Gray_Hybrid(BMP_Image *img, int rank, int size)
{
    int width = img->width;
    int height = img->height;
    int bpp = img->bytes_per_pixel;
    int row_size = width * bpp;

    int rows_per_proc = height / size;
    int remainder = height % size;

    int local_height = rows_per_proc + (rank < remainder ? 1 : 0);
    int local_size = local_height * row_size;

    unsigned char *local_data = (unsigned char *)malloc(local_size);
    int *sendcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));

    int offset = 0;
    int i;
    for (i = 0; i < size; i++)
    {
        int rows = rows_per_proc + (i < remainder ? 1 : 0);
        sendcounts[i] = rows * row_size;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    MPI_Scatterv(img->data, sendcounts, displs, MPI_UNSIGNED_CHAR,
                 local_data, local_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    int pixel;
    #pragma omp parallel for schedule(static)
    for (pixel = 0; pixel < local_size; pixel += 3)
    {
        unsigned char gray = RGB2Gray(local_data[pixel + 2], local_data[pixel + 1], local_data[pixel]);
        local_data[pixel]     = gray;
        local_data[pixel + 1] = gray;
        local_data[pixel + 2] = gray;
    }

    MPI_Gatherv(local_data, local_size, MPI_UNSIGNED_CHAR,
                img->data, sendcounts, displs, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    free(local_data);
    free(sendcounts);
    free(displs);
}