#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    if (rank == 0)
    {
        img = BMP_Open(argv[1]);
        if (img == NULL)
        {
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        printf("Loaded %s: %ux%u (%u bpp), %d MPI processes\n",
               argv[1], img->width, img->height, img->bytes_per_pixel * 8, size);
    }

    /* Zero-init to avoid UB — MPI_Bcast overwrites from root */
    int width = 0, height = 0, bpp = 0;
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
        img->data_size = width * height * bpp;
        img->data = (unsigned char *)malloc(img->data_size + 16);
    }

    double start, end;

    start = MPI_Wtime();
    BMP_Gray_Hybrid(img, rank, size);
    end = MPI_Wtime();
    if (rank == 0)
    {
        printf("Grayscale time: %.3f ms\n", 1000.0 * (end - start));
        if (BMP_Save(img, argv[2]) == 0)
        {
            printf("Output file invalid!\n");
            BMP_Destroy(img);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    start = MPI_Wtime();
    BMP_GaussianBlur_Hybrid(img, rank, size);
    end = MPI_Wtime();
    if (rank == 0)
    {
        printf("Gaussian blur time: %.3f ms\n", 1000.0 * (end - start));
        if (BMP_Save(img, argv[3]) == 0)
        {
            printf("Output file invalid!\n");
            BMP_Destroy(img);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    start = MPI_Wtime();
    BMP_Sobel_Hybrid(img, rank, size);
    end = MPI_Wtime();
    if (rank == 0)
    {
        printf("Sobel filter time: %.3f ms\n", 1000.0 * (end - start));
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
