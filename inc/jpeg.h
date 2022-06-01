#ifndef __JPEG_H
#define __JPEG_H

#include "huff.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

// float cos_lookup[8][8];
// int scan_order[64];

typedef struct __jpeg_data {
  // image dimensions
  int width;
  int height;
  int num_pixel; // = width*height

  // RGB data of the input image
  int *red;
  int *green;
  int *blue;

  unsigned int *rgb;

  // YCbCr for the colorspace-conversion
  int *y;
  int *cb;
  int *cr;

  // sub-sampled chroma
  int *cb_sub;
  int *cr_sub;

  // dct coefficients: 64 coefficients of the first block, then the second
  // block...
  double *dct_y;
  double *dct_cb;
  double *dct_cr;

  // quantized dct coefficients
  int *dct_y_quant;
  int *dct_cb_quant;
  int *dct_cr_quant;

  // huffman entropy coding parameters
  huff_code luma_dc;
  huff_code luma_ac;
  huff_code chroma_dc;
  huff_code chroma_ac;

} jpeg_data;

typedef struct {
  jpeg_data *data;
  char *name;
} dct_muti;

int decode(jpeg_data *jpg);
int encode(unsigned char *ycbcr, jpeg_data *jpg, unsigned int width,
           unsigned int height, unsigned int quality);
int rgb_to_ycbcr(jpeg_data *data);
int alloc_jpeg_data(jpeg_data *data);
double timer();

// int alloc_jpeg_data(jpeg_data *data);

#endif
