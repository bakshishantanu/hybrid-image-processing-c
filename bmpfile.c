#include <stdio.h>
#include <stdlib.h>
#include "bmpfile.h"
#define MAGIC_VALUE 0X4D42
#define BITS_PER_PIXEL 24
#define NUM_PLANE 1
#define COMPRESSION 0
#define BITS_PER_BYTE 8

static int checkHeader (BMP_Header *hdr)
{
    if (hdr->type != MAGIC_VALUE)
    {
        return 0;
    }
    if (hdr->bits_per_pixel != BITS_PER_PIXEL)
    {
        return 0;
    }
    if (hdr->colour_planes != NUM_PLANE)
    {
        return 0;
    }
    if (hdr->compression != COMPRESSION)
    {
        return 0;
    }
    return 1;
}

BMP_Image *cleanUp (FILE *fp, BMP_Image *img)
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
    FILE *fp = NULL;
    BMP_Image *img = NULL;
    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        return cleanUp(fp, img);
    }

    img = malloc(sizeof(BMP_Image));
    if (img == NULL)
    {
        fclose(fp);
        return NULL;
    }
    if (fread(&(img->header), sizeof(BMP_Header), 1, fp) != 1)
    {
        return cleanUp(fp, img);
    }
    if (checkHeader(&(img->header)) == 0)
    {
        return cleanUp(fp, img);
    }

    img->data_size = (img->header).size - sizeof(BMP_Header);
    img->width = (img->header).width;
    img->height = (img->header).height;
    img->bytes_per_pixel = (img->header).bits_per_pixel / BITS_PER_BYTE;
    img->data = malloc(sizeof(unsigned char)*(img->data_size));

    if (img->data == NULL)
    {
        return cleanUp(fp, img);
    }
    if (fread(img->data, sizeof(char), img->data_size, fp) != (img->data_size))
    {
        return cleanUp(fp, img);
    }

    fclose(fp);
    return img;
}

int BMP_Save(const BMP_Image *img, const char *filename)
{
    FILE *fp = NULL;
    fp = fopen(filename, "wb");

    if (fp == NULL)
    {
        return 0;
    }
    if (fwrite(&(img->header), sizeof(BMP_Header), 1, fp) != 1)
    {
        fclose(fp);
        return 0;
    }
    if (fwrite(img->data, sizeof(char), img->data_size, fp) != (img->data_size))
    {
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    return 1;
}

void BMP_Destroy(BMP_Image *img)
{
    free(img->data);
    free(img);
}