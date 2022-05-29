#ifndef __JPEG_ENCODER_NEON_H
#define __JPEG_ENCODER_NEON_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <arm_neon.h>

// ====================================================================================================================

/*
 * ISO/IEC 10918-1/ K.2
 */
typedef struct __huff_code
{
	int sym_freq[257];     // frequency of occurrence of symbol i
	int code_len[257];     // code length of symbol i
	int next[257];         // index to next symbol in chain of all symbols in current branch of code tree
	int code_len_freq[32]; // the frequencies of huff-symbols of length i
	int sym_sorted[256];   // the symbols to be encoded
	int sym_code_len[256]; // the huffman code length of symbol i
	int sym_code[256];     // the huffman code of the symbol i
} huff_code;

typedef struct __jpeg_data
{
	// image dimensions
	int width;
	int height;
	int num_pixel; // = width*height

	// RGB data of the input image
	int* red;
	int* green;
	int* blue;

	// YCbCr for the colorspace-conversion
	int* y;
	int* cb;
	int* cr;

	// sub-sampled chroma
	int* cb_sub;
	int* cr_sub;

	// dct coefficients: 64 coefficients of the first block, then the second block...
	double* dct_y;
	double* dct_cb;
	double* dct_cr;

	// quantized dct coefficients
	int* dct_y_quant;
	int* dct_cb_quant;
	int* dct_cr_quant;

	// huffman entropy coding parameters
	huff_code luma_dc;
	huff_code luma_ac;
	huff_code chroma_dc;
	huff_code chroma_ac;

} jpeg_data;
int encode(unsigned char* ycbcr, unsigned int width, unsigned int height, unsigned int quality);


#endif 
