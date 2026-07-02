#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmpfile.h"
#define MAGIC_VALUE 0x4D42
#define BITS_PER_PIXEL 24
#define NUM_PLANE 1
#define COMPRESSION 0
#define BITS_PER_BYTE 8

static int checkHeader (BMP_Header *hdr)
{
    if (hdr->type != MAGIC_VALUE)
    {
        fprintf(stderr, "Error: Not a BMP file (invalid magic 0x%04X)\n", hdr->type);
        return 0;
    }
    if (hdr->bits_per_pixel != BITS_PER_PIXEL)
    {
        fprintf(stderr, "Error: Only 24-bit BMP supported (got %u-bit)\n", hdr->bits_per_pixel);
        return 0;
    }
    if (hdr->colour_planes != NUM_PLANE)
    {
        fprintf(stderr, "Error: Invalid colour planes (expected 1, got %u)\n", hdr->colour_planes);
        return 0;
    }
    if (hdr->compression != COMPRESSION)
    {
        fprintf(stderr, "Error: Compressed BMP not supported (type %u)\n", hdr->compression);
        return 0;
    }
    if (hdr->width == 0 || hdr->height == 0)
    {
        fprintf(stderr, "Error: Invalid image dimensions (%ux%u)\n", hdr->width, hdr->height);
        return 0;
    }
    if (hdr->width > 65536 || hdr->height > 65536)
    {
        fprintf(stderr, "Error: Image too large (%ux%u, max 65536)\n", hdr->width, hdr->height);
        return 0;
    }
    if (hdr->offset < sizeof(BMP_Header))
    {
        fprintf(stderr, "Error: Invalid pixel data offset (%u < %zu)\n",
                hdr->offset, sizeof(BMP_Header));
        return 0;
    }
    return 1;
}

static BMP_Image *cleanUp (FILE *fp, BMP_Image *img)
{
    if (fp != NULL)
    {
        fclose(fp);
    }
    if (img != NULL)
    {
        if (img->data != NULL)
        {
            free(img->data);
        }
        free(img);
    }
    return NULL;
}

BMP_Image *BMP_Open(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }

    BMP_Image *img = malloc(sizeof(BMP_Image));
    if (img == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return NULL;
    }
    img->data = NULL;

    if (fread(&(img->header), sizeof(BMP_Header), 1, fp) != 1)
    {
        fprintf(stderr, "Error: Failed to read BMP header from '%s'\n", filename);
        return cleanUp(fp, img);
    }
    if (checkHeader(&(img->header)) == 0)
    {
        return cleanUp(fp, img);
    }

    img->width = (img->header).width;
    img->height = (img->header).height;
    img->bytes_per_pixel = (img->header).bits_per_pixel / BITS_PER_BYTE;

    unsigned int row_bytes = img->width * img->bytes_per_pixel;
    unsigned int row_stride = (row_bytes + 3) & ~3;
    unsigned int padding = row_stride - row_bytes;

    /* Store contiguous pixel data (no row padding) */
    img->data_size = row_bytes * img->height;

    /* Allocate with 16 extra bytes for safe SIMD loads at the buffer end */
    img->data = malloc(img->data_size + 16);
    if (img->data == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate %u bytes for pixel data\n", img->data_size);
        return cleanUp(fp, img);
    }
    memset(img->data + img->data_size, 0, 16);

    /* Seek to the actual pixel data offset (skips ICC profiles, colour tables, etc.) */
    if (fseek(fp, (img->header).offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error: Failed to seek to pixel data at offset %u\n", (img->header).offset);
        return cleanUp(fp, img);
    }

    /* Read pixel data, stripping BMP row padding if present */
    if (padding == 0)
    {
        if (fread(img->data, 1, img->data_size, fp) != img->data_size)
        {
            fprintf(stderr, "Error: Failed to read pixel data\n");
            return cleanUp(fp, img);
        }
    }
    else
    {
        unsigned char pad_buf[3];
        for (unsigned int y = 0; y < img->height; y++)
        {
            if (fread(img->data + y * row_bytes, 1, row_bytes, fp) != row_bytes)
            {
                fprintf(stderr, "Error: Failed to read pixel row %u\n", y);
                return cleanUp(fp, img);
            }
            if (fread(pad_buf, 1, padding, fp) != padding)
            {
                fprintf(stderr, "Error: Failed to read row padding at row %u\n", y);
                return cleanUp(fp, img);
            }
        }
    }

    fclose(fp);
    return img;
}

int BMP_Save(const BMP_Image *img, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return 0;
    }

    unsigned int row_bytes = img->width * img->bytes_per_pixel;
    unsigned int row_stride = (row_bytes + 3) & ~3;
    unsigned int padding = row_stride - row_bytes;

    /* Build a clean output header (standard BITMAPINFOHEADER, no ICC profile) */
    BMP_Header out_header = img->header;
    out_header.offset = sizeof(BMP_Header);
    out_header.header_size = 40;
    out_header.image_size = row_stride * img->height;
    out_header.size = out_header.offset + out_header.image_size;

    if (fwrite(&out_header, sizeof(BMP_Header), 1, fp) != 1)
    {
        fclose(fp);
        return 0;
    }

    /* Write pixel data with BMP row padding */
    if (padding == 0)
    {
        if (fwrite(img->data, 1, img->data_size, fp) != img->data_size)
        {
            fclose(fp);
            return 0;
        }
    }
    else
    {
        unsigned char pad_buf[3] = {0, 0, 0};
        for (unsigned int y = 0; y < img->height; y++)
        {
            if (fwrite(img->data + y * row_bytes, 1, row_bytes, fp) != row_bytes)
            {
                fclose(fp);
                return 0;
            }
            if (fwrite(pad_buf, 1, padding, fp) != padding)
            {
                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);
    return 1;
}

void BMP_Destroy(BMP_Image *img)
{
    free(img->data);
    free(img);
}