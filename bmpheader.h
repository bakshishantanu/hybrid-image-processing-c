#ifndef _bmpheader_
#define _bmpheader_
#include <stdint.h>

#pragma pack(push,1)

typedef struct
{
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t colour_planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    uint32_t xresolution;
    uint32_t yresolution;
    uint32_t ncolours;
    uint32_t important_colours;
    
}BMP_Header;

#pragma pack(pop)

#endif