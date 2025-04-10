#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>
#include "bmpimage.h"
#include "bmpfile.h"
#include "bmpfunctions.h"

int main(int argc, char **argv)
{
    if (argc < 5) 
    {
        printf("Usage: image <input> <gray> <blur> <sobel>\n");
        return EXIT_FAILURE;
    }

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    BMP_Image *img = NULL;
    clock_t start, end;

    if (rank == 0)
    {
        img = BMP_Open(argv[1]);
        if (img == NULL)
        {
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    int width, height, bpp;
    if (rank == 0)
    {
        width = img->width;
        height = img->height;
        bpp = img->bytes_per_pixel;
    }

    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&bpp, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
    {
        img = (BMP_Image *)malloc(sizeof(BMP_Image));
        img->width = width;
        img->height = height;
        img->bytes_per_pixel = bpp;
        img->data = (unsigned char *)malloc(width * height * bpp);
    }

    if (rank == 0) start = clock();
    BMP_Gray_Hybrid(img, rank, size);
    if (rank == 0)
    {
        end = clock();
        printf("Grayscale time: %.3f ms\n", 1000.0 * (end - start) / CLOCKS_PER_SEC);
        if (BMP_Save(img, argv[2]) == 0)
        {
            printf("Output file invalid!\n");
            BMP_Destroy(img);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    if (rank == 0) start = clock();
    BMP_GaussianBlur_Hybrid(img, rank, size);
    if (rank == 0)
    {
        end = clock();
        printf("Gaussian blur time: %.3f ms\n", 1000.0 * (end - start) / CLOCKS_PER_SEC);
        if (BMP_Save(img, argv[3]) == 0)
        {
            printf("Output file invalid!\n");
            BMP_Destroy(img);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    if (rank == 0) start = clock();
    BMP_Sobel_Hybrid(img, rank, size);
    if (rank == 0)
    {
        end = clock();
        printf("Sobel filter time: %.3f ms\n", 1000.0 * (end - start) / CLOCKS_PER_SEC);
        if (BMP_Save(img, argv[4]) == 0)
        {
            printf("Output file invalid!\n");
            BMP_Destroy(img);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        BMP_Destroy(img);
    }
    else
    {
        free(img->data);
        free(img);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
