#ifndef _bmpfile_
#define _bmpfile_
#include "bmpimage.h"

BMP_Image * BMP_Open (const char *filename);
int BMP_Save (const BMP_Image *img, const char *filename);
void BMP_Destroy (BMP_Image *img);

#endif