#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bmpheader.h"

#define KERNEL_SIZE 5
#define SOBEL_KERNEL_SIZE 3

/*
 * Measure system memory bandwidth by timing a large memcpy.
 * Uses 128 MB buffers, 10 iterations after warmup.
 * Returns bandwidth in GB/s.
 */
#ifdef _WIN32
#include <windows.h>
#endif

static double measure_bandwidth_gbps(void)
{
    size_t buf_size = 128 * 1024 * 1024; /* 128 MB */
    unsigned char *src = (unsigned char *)malloc(buf_size);
    unsigned char *dst = (unsigned char *)malloc(buf_size);

    if (!src || !dst)
    {
        free(src);
        free(dst);
        fprintf(stderr, "Warning: Could not allocate memory for bandwidth test\n");
        return 0.0;
    }

    /* Touch both buffers to ensure pages are mapped */
    memset(src, 0xAA, buf_size);
    memset(dst, 0x55, buf_size);

    /* Warmup pass */
    memcpy(dst, src, buf_size);

    int iters = 10;
    double seconds = 0.0;

#ifdef _WIN32
    LARGE_INTEGER freq, t_start, t_end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t_start);
    for (int i = 0; i < iters; i++)
    {
        memcpy(dst, src, buf_size);
    }
    QueryPerformanceCounter(&t_end);
    seconds = (double)(t_end.QuadPart - t_start.QuadPart) / freq.QuadPart;
#else
    clock_t c_start = clock();
    for (int i = 0; i < iters; i++)
    {
        memcpy(dst, src, buf_size);
    }
    clock_t c_end = clock();
    seconds = (double)(c_end - c_start) / CLOCKS_PER_SEC;
#endif

    double gbps = 0.0;
    if (seconds > 0.0)
    {
        gbps = (double)buf_size * iters / seconds / (1024.0 * 1024.0 * 1024.0);
    }

    free(src);
    free(dst);
    return gbps;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: perf_model <image.bmp>\n");
        printf("Computes arithmetic intensity and roofline data for each filter.\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "Error: Cannot open '%s'\n", argv[1]);
        return 1;
    }

    BMP_Header hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1)
    {
        fprintf(stderr, "Error: Cannot read BMP header\n");
        fclose(f);
        return 1;
    }
    fclose(f);

    unsigned int W = hdr.width;
    unsigned int H = hdr.height;
    double pixels = (double)W * H;
    int bpp = hdr.bits_per_pixel / 8;

    printf("====================================\n");
    printf("  Performance Model & Roofline Data\n");
    printf("====================================\n\n");
    printf("Image: %s (%ux%u, %.0f pixels, %d bytes/pixel)\n\n", argv[1], W, H, pixels, bpp);

    /* ============================
     * FLOP & Byte Count Per Filter
     * ============================ */

    /* Grayscale: per pixel = 3 multiply + 2 add = 5 FLOPs
     * Bytes: read 3 channels + write 3 channels = 6 bytes/pixel (in-place) */
    double gray_flops = 5.0 * pixels;
    double gray_bytes = 6.0 * pixels;

    /* Gaussian Blur (separable, K-tap, 2 passes):
     * Per pixel per channel per pass: K multiply + (K-1) add + 1 divide = 2K FLOPs
     * Per pass total: 2K * bpp * pixels
     * 2 passes total: 4K * bpp * pixels
     * Bytes per pass: read K neighbors * bpp + write bpp = (K+1)*bpp per pixel
     * Plus memcpy between passes: 2*bpp per pixel (read + write)
     * Total bytes: 2 * [(K+1)*bpp + 2*bpp] * pixels */
    int K = KERNEL_SIZE;
    double blur_flops = 4.0 * K * bpp * pixels;
    double blur_bytes = 2.0 * ((K + 1.0) * bpp + 2.0 * bpp) * pixels;

    /* Sobel: 2 kernels, each 3x3:
     * Per kernel per pixel: 9 multiply + 8 add = 17 FLOPs
     * 2 kernels: 34 FLOPs
     * Gradient magnitude: 2 multiply + 1 add + 1 sqrt (~15 FLOPs) + 1 compare = ~19 FLOPs
     * Total per pixel: ~53 FLOPs
     * Bytes: read 9 neighbors * bpp (shared for both kernels via cache) + grad write 2*4 bytes
     *        + grad read 2*4 bytes + pixel write bpp */
    double sobel_flops = 53.0 * pixels;
    double sobel_bytes = (9.0 * bpp + 16.0 + bpp) * pixels;

    /* Unsharp Mask: Gaussian blur + sharpening
     * Sharpening per byte: 1 subtract + 1 multiply + 1 add + 1 clamp = 4 FLOPs
     * Extra bytes: save original (2*data_size) + sharpening read/write (2*data_size) */
    double unsharp_flops = blur_flops + 4.0 * bpp * pixels;
    double unsharp_bytes = blur_bytes + 4.0 * bpp * pixels;

    /* Arithmetic Intensity = FLOPs / Bytes */
    double gray_ai    = gray_flops / gray_bytes;
    double blur_ai    = blur_flops / blur_bytes;
    double sobel_ai   = sobel_flops / sobel_bytes;
    double unsharp_ai = unsharp_flops / unsharp_bytes;

    /* Memory Bandwidth Probe */
    printf("Measuring system memory bandwidth...\n");
    double bw_gbps = measure_bandwidth_gbps();
    if (bw_gbps <= 0.0)
    {
        printf("Warning: bandwidth measurement failed, using estimate of 20 GB/s\n");
        bw_gbps = 20.0;
    }
    printf("Measured bandwidth: %.1f GB/s\n\n", bw_gbps);

    double bw_bps = bw_gbps * 1024.0 * 1024.0 * 1024.0;

    /* Results Table */
    printf("%-15s %14s %14s %12s %16s\n",
           "Filter", "FLOPs", "Bytes", "AI (F/B)", "BW-limit (ms)");
    printf("%-15s %14s %14s %12s %16s\n",
           "---------------", "--------------", "--------------",
           "------------", "----------------");
    printf("%-15s %14.0f %14.0f %12.4f %16.3f\n",
           "Grayscale", gray_flops, gray_bytes, gray_ai,
           gray_bytes / bw_bps * 1000.0);
    printf("%-15s %14.0f %14.0f %12.4f %16.3f\n",
           "Gaussian Blur", blur_flops, blur_bytes, blur_ai,
           blur_bytes / bw_bps * 1000.0);
    printf("%-15s %14.0f %14.0f %12.4f %16.3f\n",
           "Sobel", sobel_flops, sobel_bytes, sobel_ai,
           sobel_bytes / bw_bps * 1000.0);
    printf("%-15s %14.0f %14.0f %12.4f %16.3f\n",
           "Unsharp Mask", unsharp_flops, unsharp_bytes, unsharp_ai,
           unsharp_bytes / bw_bps * 1000.0);

    printf("\n");
    printf("Interpretation:\n");
    printf("  BW-limit = minimum time if the filter is purely memory-bound.\n");
    printf("  If measured time >> BW-limit, the filter is compute-bound.\n");
    printf("  If measured time ~= BW-limit, the filter is memory-bound.\n");
    printf("\n");

    /* Working Set Analysis */
    double ws_mb = pixels * bpp / (1024.0 * 1024.0);
    printf("Working set: %.1f MB (image data)\n", ws_mb);
    printf("  L1 cache (~32 KB):  %s\n", ws_mb * 1024 > 32 ? "EXCEEDS" : "fits");
    printf("  L2 cache (~256 KB): %s\n", ws_mb * 1024 > 256 ? "EXCEEDS" : "fits");
    printf("  L3 cache (~8 MB):   %s\n", ws_mb > 8 ? "EXCEEDS" : "fits");
    printf("\n");

    /* Write CSV */
#ifdef _WIN32
    system("if not exist results mkdir results");
#else
    system("mkdir -p results");
#endif

    FILE *csv = fopen("results/roofline_data.csv", "w");
    if (csv)
    {
        fprintf(csv, "filter,flops,bytes,arith_intensity,bw_limit_ms,bw_gbps\n");
        fprintf(csv, "grayscale,%.0f,%.0f,%.4f,%.4f,%.1f\n",
                gray_flops, gray_bytes, gray_ai, gray_bytes / bw_bps * 1000.0, bw_gbps);
        fprintf(csv, "gaussian_blur,%.0f,%.0f,%.4f,%.4f,%.1f\n",
                blur_flops, blur_bytes, blur_ai, blur_bytes / bw_bps * 1000.0, bw_gbps);
        fprintf(csv, "sobel,%.0f,%.0f,%.4f,%.4f,%.1f\n",
                sobel_flops, sobel_bytes, sobel_ai, sobel_bytes / bw_bps * 1000.0, bw_gbps);
        fprintf(csv, "unsharp_mask,%.0f,%.0f,%.4f,%.4f,%.1f\n",
                unsharp_flops, unsharp_bytes, unsharp_ai, unsharp_bytes / bw_bps * 1000.0, bw_gbps);
        fclose(csv);
        printf("CSV written to results/roofline_data.csv\n");
    }
    else
    {
        fprintf(stderr, "Warning: Could not create results/roofline_data.csv\n");
    }

    return 0;
}
