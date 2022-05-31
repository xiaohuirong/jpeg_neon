/**
 * Minimalistic JPEG Encoder
 *   baseline-DCT profile
 *   fixed 4:2:0 chroma subsampling
 *   dynamic luma/chroma quantization
 *   only dimensions which are multiples of 16 are supported
 *   dynamic huffman table creation
 *
 * Schier Michael, April 2011
 */

#include "jpeg.h"
extern int image_enhanced(jpeg_data *data);
// ====================================================================================================================

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLIP(n, min, max) MIN((MAX((n), (min))), (max))

float cos_lookup2[8][8] = {
    {1.0000, 0.9808, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951},
    {1.0000, 0.8315, 0.3827, -0.1951, -0.7071, -0.9808, -0.9239, -0.5556},
    {1.0000, 0.5556, -0.3827, -0.9808, -0.7071, 0.1951, 0.9239, 0.8315},
    {1.0000, 0.1951, -0.9239, -0.5556, 0.7071, 0.8315, -0.3827, -0.9808},
    {1.0000, -0.1951, -0.9239, 0.5556, 0.7071, -0.8315, -0.3827, 0.9808},
    {1.0000, -0.5556, -0.3827, 0.9808, -0.7071, -0.1951, 0.9239, -0.8315},
    {1.0000, -0.8315, 0.3827, 0.1951, -0.7071, 0.9808, -0.9239, 0.5556},
    {1.0000, -0.9808, 0.9239, -0.8315, 0.7071, -0.5556, 0.3827, -0.1951}};

int scan_order2[] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18,
                     11, 4,  5,  12, 19, 26, 33, 40, 48, 41, 34, 27, 20,
                     13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43,
                     36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45,
                     38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

/*
 * stop-watch used for time measurements
 */
double timer() {
  static double last_call = 0;
  static struct timeval arg;
  gettimeofday(&arg, NULL);
  double ret = arg.tv_sec * 1000.0 + arg.tv_usec / 1000.0 - last_call;
  last_call = arg.tv_sec * 1000.0 + arg.tv_usec / 1000.0;
  return ret;
}

// ====================================================================================================================

/*
 * converts the rgb data to ycbcr data
 */
int rgb_to_ycbcr(jpeg_data *data) {
  int i;
  for (i = 0; i < data->num_pixel; i++) {
    data->y[i] =
        0.299 * data->red[i] + 0.587 * data->green[i] + 0.114 * data->blue[i];
    data->cb[i] = 128 - 0.168736 * data->red[i] - 0.331264 * data->green[i] +
                  0.5 * data->blue[i];
    data->cr[i] = 128 + 0.5 * data->red[i] - 0.418688 * data->green[i] -
                  0.081312 * data->blue[i];
    // assert( 0<=data->y[i] && data->y[i]<=255);
    // assert( 0<=data->cb[i] && data->cb[i]<=255 );
    // assert( 0<=data->cr[i] && data->cr[i]<=255 );
    data->rgb[i] = CLIP(data->red[i], 0, 255);
    data->rgb[i] <<= 8;
    data->rgb[i] |= CLIP(data->green[i], 0, 255);
    data->rgb[i] <<= 8;
    data->rgb[i] |= CLIP(data->blue[i], 0, 255);
  }

  return 0;
}

int read_yuyv(jpeg_data *data, unsigned char *yuyv, unsigned int width,
              unsigned int height) {
  data->width = width;
  data->height = height;
  data->num_pixel = width * height;
  // alloc_jpeg_data(data);
  int i;
  int j;
  int k;
  for (i = 0; i < width * height / 4; i++) {
    k = i * 8;
    int Y0 = yuyv[k];
    int U0 = yuyv[k + 1];
    int Y1 = yuyv[k + 2];
    int V1 = yuyv[k + 3];
    int Y2 = yuyv[k + 4];
    int U2 = yuyv[k + 5];
    int Y3 = yuyv[k + 6];
    int V3 = yuyv[k + 7];

    j = 4 * i;
    data->y[j] = Y0;
    data->y[j + 1] = Y1;
    data->y[j + 2] = Y2;
    data->y[j + 3] = Y3;

    data->cb[j] = U0;
    data->cb[j + 1] = U0;
    data->cb[j + 2] = U2;
    data->cb[j + 3] = U2;

    data->cr[j] = V1;
    data->cr[j + 1] = V1;
    data->cr[j + 2] = V3;
    data->cr[j + 3] = V3;
  }
  return 0;
}

int read_ycbcr(jpeg_data *data, unsigned char *ycbcr, unsigned int width,
               unsigned int height) {

  data->width = width;
  data->height = height;
  data->num_pixel = width * height;
  // alloc_jpeg_data(data);
  int i;
  int j;
  for (i = 0; i < width * height; i++) {
    data->y[i] = ycbcr[i];
  }

  j = width * height;
  for (i = 0; i < width * height / 4; i++) {
    data->cb_sub[i] = ycbcr[j + 2 * i];
    data->cr_sub[i] = ycbcr[j + 2 * i + 1];
  }
  return 0;
}
// ====================================================================================================================

/*
 * subsamples the cb and cr channels (to c?_sub)
 */
void subsample_chroma(jpeg_data *data) {
  int h, w;
  for (h = 0; h < data->height / 2; h++)
    for (w = 0; w < data->width / 2; w++) {
      int i = 2 * h * data->width + 2 * w;
      data->cb_sub[h * data->width / 2 + w] =
          (data->cb[i] + data->cb[i + 1] + data->cb[i + data->width] +
           data->cb[i + data->width + 1]) /
          4;
      data->cr_sub[h * data->width / 2 + w] =
          (data->cr[i] + data->cr[i + 1] + data->cr[i + data->width] +
           data->cr[i + data->width + 1]) /
          4;
      assert(0 <= data->cb_sub[h * data->width / 2 + w] &&
             data->cb_sub[h * data->width / 2 + w] <= 255);
      assert(0 <= data->cr_sub[h * data->width / 2 + w] &&
             data->cr_sub[h * data->width / 2 + w] <= 255);
    }
}

/*
 * the discrete cosine transform per 8x8 block - outputs floating point values
 * optimized by using lookup tables and splitting the terms
 */
/*
 * the discrete cosine transform per 8x8 block - outputs floating point values
 * optimized by using lookup tables and splitting the terms
 */

void dct_block(int gap, int in[], double out[]) {
  int x_f, y_f; // frequency domain coordinates
  int x_t, y_t; // time domain coordinates

  float inner_lookup[8][8];
  for (x_t = 0; x_t < 8; x_t++)
    for (y_f = 0; y_f < 8; y_f++) {
      inner_lookup[x_t][y_f] = 0;
      for (y_t = 0; y_t < 8; y_t++)
        inner_lookup[x_t][y_f] +=
            (in[y_t * gap + x_t] - 128) * cos_lookup2[y_t][y_f];
    }

  // freq(x_f,y_f) = ...
  float freq;
  for (y_f = 0; y_f < 8; y_f++)
    for (x_f = 0; x_f < 8; x_f++) {
      freq = 0;
      for (x_t = 0; x_t < 8; x_t++)
        freq += inner_lookup[x_t][y_f] * cos_lookup2[x_t][x_f];

      if (x_f == 0)
        freq *= M_SQRT1_2;
      if (y_f == 0)
        freq *= M_SQRT1_2;
      freq /= 4;

      out[y_f * 8 + x_f] = freq;
    }
}

/*
 * perform the discrete cosine transform
 */
void *dct_mutiphread(void *args) {
  dct_muti *dct = (dct_muti *)args;
  int h, v, blocks_vert, blocks_horiz;
  if (dct->name == "y") {
    blocks_vert = (dct->data->height) / 8;
    blocks_horiz = (dct->data->width) / 8;
    for (v = 0; v < blocks_vert; v++)
      for (h = 0; h < blocks_horiz; h++)
        dct_block(8 * blocks_horiz,
                  dct->data->y + v * blocks_horiz * 64 + h * 8,
                  dct->data->dct_y + (v * blocks_horiz + h) * 64);

  } else if (dct->name == "cb") {
    blocks_vert = (dct->data->height) / 16;
    blocks_horiz = (dct->data->width) / 16;
    for (v = 0; v < blocks_vert; v++)
      for (h = 0; h < blocks_horiz; h++)
        dct_block(8 * blocks_horiz,
                  dct->data->cb + v * blocks_horiz * 64 + h * 8,
                  dct->data->dct_cb + (v * blocks_horiz + h) * 64);

  } else if (dct->name == "cr") {
    blocks_vert = (dct->data->height) / 16;
    blocks_horiz = (dct->data->width) / 16;
    for (v = 0; v < blocks_vert; v++)
      for (h = 0; h < blocks_horiz; h++)
        dct_block(8 * blocks_horiz,
                  dct->data->cr + v * blocks_horiz * 64 + h * 8,
                  dct->data->dct_cr + (v * blocks_horiz + h) * 64);
  }
}
void dct(int blocks_horiz, int blocks_vert, int in[], double out[]) {
  int h, v;
  for (v = 0; v < blocks_vert; v++)
    for (h = 0; h < blocks_horiz; h++)
      dct_block(8 * blocks_horiz, in + v * blocks_horiz * 64 + h * 8,
                out + (v * blocks_horiz + h) * 64);
}

// ====================================================================================================================

/*
 * Luma quantization matrix
 * taken from CCITT Rec. T.81
 */
int luma_quantizer2[] = {
    16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
    14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
    18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};

/*
 * Chroma quantization matrix
 */
int chroma_quantizer2[] = {17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99,
                           99, 99, 99, 24, 26, 56, 99, 99, 99, 99, 99, 47, 66,
                           99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                           99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                           99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

void set_quality(int quantizer[], int quality) {
  int i;
  for (i = 0; i < 64; i++)
    quantizer[i] = CLIP((100 - quality) / 50.0 * quantizer[i], 1, 255);
}

/*
 * quantize the DCT coefficients
 * and reorder them according to the zig-zag scan mode
 */
void quantize(int num_pixel, double in[], int out[], int quantizer[]) {
  int i;
  for (i = 0; i < num_pixel; i++) {
    out[i] = in[i] / quantizer[i % 64];
    out[i] = CLIP(out[i], -2048, 2047);
  }
}

// ====================================================================================================================

/*
 * Reorder elements in zig-zag
 * new[i] = old[scan_order2[i]]
 */
/*
 * Reorders the dct coefficients in zig-zag order using the scan_order2 matrix
 */
void zigzag(int num_pixel, int dct[]) {
  int i, j;
  int tmp[64];
  for (i = 0; i < num_pixel; i += 64) {
    for (j = 0; j < 64; j++)
      tmp[j] = dct[i + j];
    for (j = 0; j < 64; j++)
      dct[i + j] = tmp[scan_order2[j]];
  }
}

// ====================================================================================================================

/*
 * turn the absolute values of DC coefficients to differential values
 * diff(i) = dc(i) - dc(i-1)
 */
void diff_dc(int num_pixel, int dct_quant[]) {
  int i;
  int last = dct_quant[0];
  for (i = 64; i < num_pixel; i += 64) {
    dct_quant[i] -= last;
    last += dct_quant[i];
  }
}

// ====================================================================================================================

/*
 * construct the huffman table from the derived frequency distribution
 */

void init_huff_table(huff_code *hc) {
  int i;
  for (i = 0; i < 257; i++) {
    hc->code_len[i] = 0;
    hc->next[i] = -1;
  }

  // derive the code length for each symbol
  while (1) {
    int v1 = -1;
    int v2 = -1;
    int i;
    // find least value of freq(v1) and next least value of freq(v2)
    for (i = 0; i < 257; i++) {
      if (hc->sym_freq[i] == 0)
        continue;
      if (v1 == -1 || hc->sym_freq[i] <= hc->sym_freq[v1]) {
        v2 = v1;
        v1 = i;
      } else if (v2 == -1 || hc->sym_freq[i] <= hc->sym_freq[v2])
        v2 = i;
    }
    if (v2 == -1)
      break;

    hc->sym_freq[v1] += hc->sym_freq[v2];
    hc->sym_freq[v2] = 0;
    while (1) {
      hc->code_len[v1]++;
      if (hc->next[v1] == -1)
        break;
      v1 = hc->next[v1];
    }
    hc->next[v1] = v2;
    while (1) {
      hc->code_len[v2]++;
      if (hc->next[v2] == -1)
        break;
      v2 = hc->next[v2];
    }
  }

  for (i = 0; i < 32; i++)
    hc->code_len_freq[i] = 0;

  // derive code length frequencies
  for (i = 0; i < 257; i++)
    if (hc->code_len[i] != 0)
      hc->code_len_freq[hc->code_len[i]]++;

  // limit the huffman code length to 16 bits
  i = 31;
  while (1) {
    if (hc->code_len_freq[i] > 0) // if the code is too long ...
    {
      int j = i - 1;
      while (hc->code_len_freq[--j] <= 0)
        ; // ... we search for an upper layer containing leaves
      hc->code_len_freq[i] -=
          2; // we remove the two leaves from the lowest layer
      hc->code_len_freq[i - 1]++;    // and put one of them one position higher
      hc->code_len_freq[j + 1] += 2; // the other one goes at a new branch ...
      hc->code_len_freq[j]--; // ... together with the leave node which was
                              // there before
      continue;
    }
    i--;
    if (i != 16)
      continue;
    while (hc->code_len_freq[i] == 0)
      i--;
    hc->code_len_freq[i]--; // remove one leave from the lowest layer (the
                            // bottom symbol '111...111')
    break;
  }

  // sort the input symbols according to their code size
  for (i = 0; i < 256; i++)
    hc->sym_sorted[i] = -1;
  int j;
  int k = 0;
  for (i = 1; i < 32; i++)
    for (j = 0; j < 256; j++)
      if (hc->code_len[j] == i)
        hc->sym_sorted[k++] = j;

  // determine the size of the huffman code symbols - this may differ from
  // code_len because of the 16 bit limit
  for (i = 0; i < 256; i++)
    hc->sym_code_len[i] = 0;
  k = 0;
  for (i = 1; i <= 16; i++)
    for (j = 1; j <= hc->code_len_freq[i]; j++)
      hc->sym_code_len[hc->sym_sorted[k++]] = i;
  hc->sym_code_len[hc->sym_sorted[k]] = 0;

  // generate the codes for the symbols
  for (i = 0; i < 256; i++)
    hc->sym_code[i] = -1;
  k = 0;
  int code = 0;
  int si = hc->sym_code_len[hc->sym_sorted[0]];
  while (1) {
    do {
      hc->sym_code[hc->sym_sorted[k]] = code;
      k++;
      code++;
    } while (hc->sym_code_len[hc->sym_sorted[k]] == si);
    if (hc->sym_code_len[hc->sym_sorted[k]] == 0)
      break;
    do {
      code <<= 1;
      si++;
    } while (hc->sym_code_len[hc->sym_sorted[k]] != si);
  }

  int len;
  for (i = 1; i <= 16; i++) {
    if (hc->code_len_freq[i] != 0) {
      len = i;
      break;
    }
  }

  int lcode;
  for (i = 0; i < 256; i++) {
    if (hc->sym_code_len[i] == len) {
      lcode = hc->sym_code[i];
      break;
    }
  }

  for (i = 0; i < 256; i++) {
    if (hc->sym_code[i] == -1) {
      hc->sym_code[i] = lcode;
      hc->sym_code_len[i] = len;
    }
  }
}

/*
 * map the value to its class
 * Class-id           DC/AC-coeff delta
 * -----------------------------------------
 *  0                        0
 *  1                      -1,1
 *  2                   -3,-2,2,3
 *  3             -7,-6,-5,-4,4,5,6,7
 *  4              -15,...,-8,8,...,15
 *  5             -31,...,-16,16,...31
 *  6             -63,...,-32,32,...63
 *  7            -127,...,-64,64,...,127
 *  8           -255,...,-128,128,...,255
 *  9           -511,...,-256,256,...,511
 * 10          -1023,...,-512,512,...,1023
 * 11         -2047,...,-1024,1024,...,2047
 */
int huff_class(int value) {
  value = (value < 0) ? (-value) : value;
  int cla = 0;
  while (value > 0) {
    value = value >> 1;
    cla++;
  }
  return cla;
}

/*
 * determine frequencies (DC) - mapped to classes
 */
void calc_dc_freq(int num_pixel, int dct_quant[], int freq[]) {
  int i;
  for (i = 0; i < num_pixel; i += 64)
    freq[huff_class(dct_quant[i])]++;
}

/*
 * determine frequencies (num_zeros + AC) - bit0-3=num_preceding_zeros,
 * bit4-7=class-id
 */
void calc_ac_freq(int num_pixel, int dct_quant[], int freq[]) {
  int i;
  int num_zeros = 0;
  int last_nonzero;
  for (i = 0; i < num_pixel; i++) {
    if (i % 64 == 0) {
      for (last_nonzero = i + 63; last_nonzero > i; last_nonzero--)
        if (dct_quant[last_nonzero] != 0)
          break;
      continue;
    }

    if (i == last_nonzero + 1) {
      freq[0x00]++; // EOB byte
      // jump to the next block
      i = (i / 64 + 1) * 64 - 1;
      continue;
    }

    if (dct_quant[i] == 0) {
      num_zeros++;
      if (num_zeros == 16) {
        freq[0xF0]++; // ZRL byte
        num_zeros = 0;
      }
      continue;
    }

    freq[((num_zeros << 4) & 0xF0) | (huff_class(dct_quant[i]) & 0x0F)]++;
    num_zeros = 0;
  }
}

/*
 * construct 4 huffman tables for DC/(num_zeros+AC) luma/chroma coefficients
 */
void init_huffman(jpeg_data *data) {
  int i;

  huff_code *luma_dc = &data->luma_dc;
  huff_code *luma_ac = &data->luma_ac;
  huff_code *chroma_dc = &data->chroma_dc;
  huff_code *chroma_ac = &data->chroma_ac;

  // initialize
  for (i = 0; i < 257; i++)
    luma_dc->sym_freq[i] = luma_ac->sym_freq[i] = chroma_dc->sym_freq[i] =
        chroma_ac->sym_freq[i] = 0;
  // reserve one code point
  luma_dc->sym_freq[256] = luma_ac->sym_freq[256] = chroma_dc->sym_freq[256] =
      chroma_ac->sym_freq[256] = 1;

  // calculate frequencies as basis for the huffman table construction
  calc_dc_freq(data->num_pixel, data->dct_y_quant, luma_dc->sym_freq);
  calc_ac_freq(data->num_pixel, data->dct_y_quant, luma_ac->sym_freq);
  calc_dc_freq(data->num_pixel / 4, data->dct_cb_quant, chroma_dc->sym_freq);
  calc_dc_freq(data->num_pixel / 4, data->dct_cr_quant, chroma_dc->sym_freq);
  calc_ac_freq(data->num_pixel / 4, data->dct_cb_quant, chroma_ac->sym_freq);
  calc_ac_freq(data->num_pixel / 4, data->dct_cr_quant, chroma_ac->sym_freq);
  timer();
  printf("Initializing the luma DC Huffman tables  ");
  init_huff_table(luma_dc);
  printf("%10.3f ms\n", timer());

  printf("Initializing the luma AC Huffman tables  ");
  init_huff_table(luma_ac);
  printf("%10.3f ms\n", timer());

  printf("Initializing the chroma DC Huffman tables");
  init_huff_table(chroma_dc);
  printf("%10.3f ms\n", timer());

  printf("Initializing the chroma AC Huffman tables");
  init_huff_table(chroma_ac);
  printf("%10.3f ms\n", timer());
}

// ====================================================================================================================

unsigned char byte_buffer;
int bits_written;
void write_byte(FILE *f, int code_word, int start, int end) {
  if (start == end)
    return;

  if (end > 0) // we just write into the buffer
  {
    code_word <<= end;
    code_word &= (1 << start) - 1;
    byte_buffer |= code_word;
    bits_written += start - end;
  } else // we have to split & write to the disk
  {
    int part2 = code_word & ((1 << (-end)) - 1);
    code_word >>= (-end);
    code_word &= (1 << start) - 1;
    byte_buffer |= code_word;
    fputc(byte_buffer, f);
    if (byte_buffer == 0xFF)
      fputc(0, f); // stuffing bit
    bits_written = 0;
    byte_buffer = 0;
    write_byte(f, part2, 8, 8 + end);
    return;
  }
}

/**
 * write code_len bits to the file (using a buffer inbetween) starting with the
 * MSB (leftmost one) example: write_bits(14, 6) writes the bits '001110'
 */
void write_bits(FILE *f, int code_word, int code_len) {
  if (code_len == 0)
    return;
  write_byte(f, code_word, 8 - bits_written, 8 - bits_written - code_len);
}

/*
 * extend the bitstream to 8-bit precision - fill missing bits with '1'
 */
void fill_last_byte(FILE *f) {
  byte_buffer |= (1 << (8 - bits_written)) - 1;
  fputc(byte_buffer, f);
  byte_buffer = 0;
  bits_written = 0;
}

/*
 * write the bit representation of this DC coefficient
 */
void encode_dc_value(FILE *f, int dc_val, huff_code *huff) {
  int cla = huff_class(dc_val);
  int class_code = huff->sym_code[cla];
  int class_size = huff->sym_code_len[cla];
  write_bits(f, class_code, class_size);

  unsigned int id = abs(dc_val);
  if (dc_val < 0)
    id = ~id;
  write_bits(f, id, cla);
}

/*
 * write the bit representation of this AC coefficient, which has num_zeros
 * preceeding zeros
 */
void encode_ac_value(FILE *f, int ac_val, int num_zeros, huff_code *huff) {

  int cla = huff_class(ac_val);
  int v = ((num_zeros << 4) & 0xF0) | (cla & 0x0F);
  int code = huff->sym_code[v];
  int size = huff->sym_code_len[v];
  write_bits(f, code, size);

  unsigned int id = abs(ac_val);
  if (ac_val < 0)
    id = ~id;
  write_bits(f, id, cla);
}

/*
 * write the DC and AC coefficients of one color channel
 */
void write_coefficients(FILE *f, int num_pixel, int dct_quant[],
                        huff_code *huff_dc, huff_code *huff_ac) {
  int num_zeros = 0;
  int last_nonzero;
  int i;
  for (i = 0; i < num_pixel; i++) {
    if (i % 64 == 0) {
      encode_dc_value(f, dct_quant[i], huff_dc);
      for (last_nonzero = i + 63; last_nonzero > i; last_nonzero--)
        if (dct_quant[last_nonzero] != 0)
          break;
      continue;
    }

    if (i == last_nonzero + 1) {
      write_bits(f, huff_ac->sym_code[0x00],
                 huff_ac->sym_code_len[0x00]); // EOB symbol
      // jump to the next block
      i = (i / 64 + 1) * 64 - 1;
      continue;
    }

    if (dct_quant[i] == 0) {
      num_zeros++;
      if (num_zeros == 16) {
        write_bits(f, huff_ac->sym_code[0xF0],
                   huff_ac->sym_code_len[0xF0]); // ZRL symbol
        num_zeros = 0;
      }
      continue;
    }

    encode_ac_value(f, dct_quant[i], num_zeros, huff_ac);
    num_zeros = 0;
  }
}

// ====================================================================================================================

/*
 * write the information from which decoders can reconstruct the huffman table
 * used
 */
void write_dht_header(FILE *f, int code_len_freq[], int sym_sorted[],
                      int tc_th) {
  int length = 19;
  int i;
  for (i = 1; i <= 16; i++)
    length += code_len_freq[i];

  fputc(0xFF, f);
  fputc(0xC4, f); // DHT Symbol

  fputc((length >> 8) & 0xFF, f);
  fputc(length & 0xFF, f); // len

  fputc(tc_th, f); // table class (0=DC, 1=AC) and table id (0=luma, 1=chroma)

  for (i = 1; i <= 16; i++)
    fputc(code_len_freq[i], f); // number of codes of length i

  for (i = 0; length > 19; i++, length--)
    fputc(sym_sorted[i],
          f); // huffval, needed to reconstruct the huffman code at the receiver
}

/*
 * write mandatory header stuff
 * - image dimensions
 * - quantization tables
 * - huffman tables
 */
void write_file(const char *file_name, jpeg_data *data) {
  FILE *f = fopen(file_name, "w");

  fputc(0xFF, f);
  fputc(0xD8, f); // SOI Symbol

  fputc(0xFF, f);
  fputc(0xE0, f); // APP0 Tag
  fputc(0, f);
  fputc(16, f); // len
  fputc(0x4A, f);
  fputc(0x46, f);
  fputc(0x49, f);
  fputc(0x46, f);
  fputc(0x00, f); // JFIF ID
  fputc(0x01, f);
  fputc(0x01, f); // JFIF Version
  fputc(0x00, f); // units
  fputc(0x00, f);
  fputc(0x48, f); // X density
  fputc(0x00, f);
  fputc(0x48, f); // Y density
  fputc(0x00, f); // x thumbnail
  fputc(0x00, f); // y thumbnail

  fputc(0xFF, f);
  fputc(0xDB, f); // DQT Symbol
  fputc(0, f);
  fputc(67, f); // len
  fputc(0, f);  // quant-table id
  int i;
  for (i = 0; i < 64; i++)
    fputc(luma_quantizer2[scan_order2[i]], f);

  fputc(0xFF, f);
  fputc(0xDB, f); // DQT Symbol
  fputc(0, f);
  fputc(67, f); // len
  fputc(1, f);  // quant-table id
  for (i = 0; i < 64; i++)
    fputc(chroma_quantizer2[scan_order2[i]], f);

  write_dht_header(f, data->luma_dc.code_len_freq, data->luma_dc.sym_sorted,
                   0x00);
  write_dht_header(f, data->luma_ac.code_len_freq, data->luma_ac.sym_sorted,
                   0x10);
  write_dht_header(f, data->chroma_dc.code_len_freq, data->chroma_dc.sym_sorted,
                   0x01);
  write_dht_header(f, data->chroma_ac.code_len_freq, data->chroma_ac.sym_sorted,
                   0x11);

  fputc(0xFF, f);
  fputc(0xC0, f); // SOF0 Symbol (Baseline DCT)
  fputc(0, f);
  fputc(17, f);   // len
  fputc(0x08, f); // data precision - 8bit
  fputc(((data->height) >> 8) & 0xFF, f);
  fputc((data->height) & 0xFF, f); // picture height
  fputc(((data->width) >> 8) & 0xFF, f);
  fputc((data->width) & 0xFF, f); // picture width
  fputc(0x03, f);                 // num components - 3 for y, cb and cr
  //		fputc(0x01, f); // num components - 3 for y, cb and cr
  fputc(1, f);    // #1 id
  fputc(0x22, f); // sampling factor (bit0-3=vertical, bit4-7=horiz)
  fputc(0, f);    // quantization table index
  fputc(2, f);    // #2 id
  fputc(0x11, f); // sampling factor (bit0-3=vertical, bit4-7=horiz)
  fputc(1, f);    // quantization table index
  fputc(3, f);    // #3 id
  fputc(0x11, f); // sampling factor (bit0-3=vertical, bit4-7=horiz)
  fputc(1, f);    // quantization table index

  fputc(0xFF, f);
  fputc(0xDA, f); // SOS Symbol
  fputc(0, f);
  fputc(8, f);    // len
  fputc(1, f);    // number of components
  fputc(1, f);    // id of component
  fputc(0x00, f); // table index, bit0-3=AC-table, bit4-7=DC-table
  fputc(0x00, f); // start of spectral or predictor selection - not used
  fputc(0x3F, f); // end of spectral selection - default value
  fputc(0x00, f); // successive approximation bits - default value
  write_coefficients(f, data->num_pixel, data->dct_y_quant, &data->luma_dc,
                     &data->luma_ac);
  fill_last_byte(f);

  fputc(0xFF, f);
  fputc(0xDA, f); // SOS Symbol
  fputc(0, f);
  fputc(8, f);    // len
  fputc(1, f);    // number of components
  fputc(2, f);    // id of component
  fputc(0x11, f); // table index, bit0-3=AC-table, bit4-7=DC-table
  fputc(0x00, f); // start of spectral or predictor selection - not used
  fputc(0x3F, f); // end of spectral selection - default value
  fputc(0x00, f); // successive approximation bits - default value
  write_coefficients(f, data->num_pixel / 4, data->dct_cb_quant,
                     &data->chroma_dc, &data->chroma_ac);
  fill_last_byte(f);

  fputc(0xFF, f);
  fputc(0xDA, f); // SOS Symbol
  fputc(0, f);
  fputc(8, f);    // len
  fputc(1, f);    // number of components
  fputc(3, f);    // id of component
  fputc(0x11, f); // table index, bit0-3=AC-table, bit4-7=DC-table
  fputc(0x00, f); // start of spectral or predictor selection - not used
  fputc(0x3F, f); // end of spectral selection - default value
  fputc(0x00, f); // successive approximation bits - default value
  write_coefficients(f, data->num_pixel / 4, data->dct_cr_quant,
                     &data->chroma_dc, &data->chroma_ac);
  fill_last_byte(f);

  fputc(0xFF, f);
  fputc(0xD9, f); // EOI Symbol

  fclose(f);
}

// ====================================================================================================================

/*
 * main routine - one cmd line parameter required: a portable pixmap file
 * (binary format)
 */
// int run(int argc, char** args)

int encode(unsigned char *ycbcr, jpeg_data *data, unsigned int width,
           unsigned int height, unsigned int quality) {
  set_quality(luma_quantizer2, quality);
  set_quality(chroma_quantizer2, quality);

  // timer();
  // printf("Converting RGB to YCbCr                  ");
  // fflush(stdout);
  // if (rgb_to_ycbcr(&data))
  // return -1;
  // printf("%10.3f ms\n", timer());

  timer();
  printf("ReadYCbCr                                ");
  fflush(stdout);
  read_yuyv(data, ycbcr, width, height);
  printf("%10.3f ms\n", timer());
  /*****************对ycbcr进行增强****************/

  image_enhanced(data);
  rgb_to_ycbcr(data);

  /*************************************************/
  timer();
  printf("Subsampling chroma values                ");
  fflush(stdout);
  subsample_chroma(data);
  printf("%10.3f ms\n", timer());

  timer();
  printf("Performing the discrete cosine transform ");
  fflush(stdout);

  /*****************多线程计算********************/

  pthread_t th1, th2, th3;
  dct_muti muti_y, muti_cb, muti_cr;
  //参数初始化
  muti_y.data = data;
  muti_y.name = "y";
  muti_cb.data = data;
  muti_cb.name = "cb";
  muti_cr.data = data;
  muti_cr.name = "cr";
  //线程创建
  pthread_create(&th1, NULL, dct_mutiphread, &muti_y);
  pthread_create(&th2, NULL, dct_mutiphread, &muti_cb);
  pthread_create(&th3, NULL, dct_mutiphread, &muti_cr);
  //等待线程结束
  pthread_join(th1, NULL);
  pthread_join(th2, NULL);
  pthread_join(th3, NULL);

  /***********************************************/
  /*****************原处理进程********************/
  /*
  dct(data.width/8, data.height/8, data.y, data.dct_y);
  dct(data.width/16, data.height/16, data.cb_sub, data.dct_cb);
  dct(data.width/16, data.height/16, data.cr_sub, data.dct_cr);
  */
  /***********************************************/
  printf("%10.3f ms\n", timer());

  timer();
  printf("Quantizing coefficients                  ");
  fflush(stdout);
  quantize(data->num_pixel, data->dct_y, data->dct_y_quant, luma_quantizer2);
  quantize(data->num_pixel / 4, data->dct_cb, data->dct_cb_quant,
           chroma_quantizer2);
  quantize(data->num_pixel / 4, data->dct_cr, data->dct_cr_quant,
           chroma_quantizer2);
  printf("%10.3f ms\n", timer());

  timer();
  printf("Reordering coefficients (zig-zag)        ");
  fflush(stdout);
  zigzag(data->num_pixel, data->dct_y_quant);
  zigzag(data->num_pixel / 4, data->dct_cb_quant);
  zigzag(data->num_pixel / 4, data->dct_cr_quant);
  printf("%10.3f ms\n", timer());

  timer();
  printf("Calculating the DC differences           ");
  fflush(stdout);
  diff_dc(data->num_pixel, data->dct_y_quant);
  diff_dc(data->num_pixel / 4, data->dct_cb_quant);
  diff_dc(data->num_pixel / 4, data->dct_cr_quant);
  printf("%10.3f ms\n", timer());

  init_huffman(data);

  timer();
  printf("Writing the bitstream                    ");
  fflush(stdout);
  write_file("out.jpg", data);
  printf("%10.3f ms\n", timer());

  return 0;
}
