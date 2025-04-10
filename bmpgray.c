#include "bmpfunctions.h"
#include <stdio.h>

static int RGB2Gray (unsigned char red, unsigned char green, unsigned char blue)
{
    double gray = 0.299*red + 0.587*green + 0.114*blue;
    return (int)(gray + 0.5);
}

void BMP_Gray (BMP_Image *img)
{
    for (int pixel = 0; pixel < img->data_size; pixel+=3)
    {
        unsigned char gray = RGB2Gray(img->data[pixel+2], img->data[pixel+1], img->data[pixel]);
        img->data[pixel+2] = gray;
        img->data[pixel+1] = gray;
        img->data[pixel] = gray;
    }
}