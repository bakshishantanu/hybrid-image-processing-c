#ifndef _bmpfunctions_
#define _bmpfunctions_
#include "bmpimage.h"

#ifdef __cplusplus
extern "C" {
#endif

void BMP_Gray (BMP_Image *img);
void BMP_Gray_Hybrid (BMP_Image *img, int rank, int size);
void BMP_GaussianBlur (BMP_Image *img);
void BMP_GaussianBlur_Hybrid (BMP_Image *img, int rank, int size);
void BMP_Sobel (BMP_Image *img);
void BMP_Sobel_Hybrid (BMP_Image *img, int rank, int size);
void BMP_UnsharpMask (BMP_Image *img);
void BMP_UnsharpMask_Hybrid (BMP_Image *img, int rank, int size);

#ifdef __cplusplus
}
#endif

#endif