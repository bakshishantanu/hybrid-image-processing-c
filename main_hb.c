#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#include "bmpimage.h"
#include "bmpfile.h"
#include "bmpfunctions.h"

int main(int argc, char **argv)
{
    /* Check for --csv flag before MPI_Init */
    int csv_mode = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--csv") == 0)
        {
            csv_mode = 1;
            for (int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argc--;
            break;
        }
    }

    if (argc < 6) 
    {
        printf("Usage: image <input> <gray> <blur> <sobel> <unsharp> [--csv]\n");
        return EXIT_FAILURE;
    }

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    BMP_Image *img = NULL;
    unsigned char *original_data = NULL;

    if (rank == 0)
    {
        img = BMP_Open(argv[1]);
        if (img == NULL)
        {
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        if (!csv_mode)
        {
            printf("Loaded %s: %ux%u (%u bpp), %d MPI processes, %d OMP threads\n",
                   argv[1], img->width, img->height, img->bytes_per_pixel * 8,
                   size, omp_get_max_threads());
        }

        /* Save original for unsharp mask */
        original_data = (unsigned char *)malloc(img->data_size);
        if (original_data)
        {
            memcpy(original_data, img->data, img->data_size);
        }
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
    double gray_ms, blur_ms, sobel_ms, unsharp_ms;

    /* Grayscale */
    start = MPI_Wtime();
    BMP_Gray_Hybrid(img, rank, size);
    end = MPI_Wtime();
    gray_ms = 1000.0 * (end - start);
    if (rank == 0)
    {
        if (!csv_mode) printf("Grayscale time: %.3f ms\n", gray_ms);
        if (BMP_Save(img, argv[2]) == 0)
        {
            printf("Output file invalid!\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    /* Gaussian Blur */
    start = MPI_Wtime();
    BMP_GaussianBlur_Hybrid(img, rank, size);
    end = MPI_Wtime();
    blur_ms = 1000.0 * (end - start);
    if (rank == 0)
    {
        if (!csv_mode) printf("Gaussian blur time: %.3f ms\n", blur_ms);
        if (BMP_Save(img, argv[3]) == 0)
        {
            printf("Output file invalid!\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    /* Sobel */
    start = MPI_Wtime();
    BMP_Sobel_Hybrid(img, rank, size);
    end = MPI_Wtime();
    sobel_ms = 1000.0 * (end - start);
    if (rank == 0)
    {
        if (!csv_mode) printf("Sobel filter time: %.3f ms\n", sobel_ms);
        if (BMP_Save(img, argv[4]) == 0)
        {
            printf("Output file invalid!\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        /* Restore original image for unsharp mask */
        if (original_data)
        {
            memcpy(img->data, original_data, img->data_size);
            free(original_data);
            original_data = NULL;
        }
    }

    /* Unsharp Mask */
    start = MPI_Wtime();
    BMP_UnsharpMask_Hybrid(img, rank, size);
    end = MPI_Wtime();
    unsharp_ms = 1000.0 * (end - start);
    if (rank == 0)
    {
        if (!csv_mode) printf("Unsharp mask time: %.3f ms\n", unsharp_ms);
        if (BMP_Save(img, argv[5]) == 0)
        {
            printf("Output file invalid!\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    /* Summary / CSV output */
    if (rank == 0)
    {
        double total_ms = gray_ms + blur_ms + sobel_ms + unsharp_ms;

        if (csv_mode)
        {
            int threads = omp_get_max_threads();
            printf("hybrid,%s,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d\n",
                   argv[1], img->width, img->height,
                   img->width * img->height,
                   gray_ms, blur_ms, sobel_ms, unsharp_ms, total_ms,
                   threads, size);
        }
        else
        {
            printf("Total time: %.3f ms\n", total_ms);
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
