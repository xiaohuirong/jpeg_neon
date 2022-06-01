#include "jpeg.h"
#include <arm_neon.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLIP(n, min, max) MIN((MAX((n), (min))), (max))


float cos_lookup[8][8] = {
    {1.0000, 0.9808, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951},
    {1.0000, 0.8315, 0.3827, -0.1951, -0.7071, -0.9808, -0.9239, -0.5556},
    {1.0000, 0.5556, -0.3827, -0.9808, -0.7071, 0.1951, 0.9239, 0.8315},
    {1.0000, 0.1951, -0.9239, -0.5556, 0.7071, 0.8315, -0.3827, -0.9808},
    {1.0000, -0.1951, -0.9239, 0.5556, 0.7071, -0.8315, -0.3827, 0.9808},
    {1.0000, -0.5556, -0.3827, 0.9808, -0.7071, -0.1951, 0.9239, -0.8315},
    {1.0000, -0.8315, 0.3827, 0.1951, -0.7071, 0.9808, -0.9239, 0.5556},
    {1.0000, -0.9808, 0.9239, -0.8315, 0.7071, -0.5556, 0.3827, -0.1951}};

int scan_order[] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18,
                    11, 4,  5,  12, 19, 26, 33, 40, 48, 41, 34, 27, 20,
                    13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43,
                    36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45,
                    38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

int luma_quantizer[64];
int chroma_quantizer[64];




int printfcode(int code, int code_len) {
  int i;
  if (code_len <= 0 | code == -1) {
    return 0;
  }
  for (i = code_len - 1; i >= 0; i--) {
    printf("%d", (code >> i) & 0x01);
  }
  return 0;
}

int printqua(int *qua) {
  int i;
  for (i = 0; i < 64; i++) {
    if (i % 8 == 0)
      printf("\n");
    printf("%d ", qua[i]);
  }
  printf("\n");
  return 0;
}

int printfclen(huff_code *hc) {
  int i = 0;
  int k = 0;
  for (i = 0; i < 256; i++) {
    if (hc->sym_code_len[i] != 0) {
      if (k % 8 == 0) {
        printf("\n");
      }
      printf("(%d)%d{", i, hc->sym_code_len[i]);
      printfcode(hc->sym_code[i], hc->sym_code_len[i]);
      printf("} ");
      k++;
    }
  }
  printf("\n");
  return 0;
}

int recoverquality(FILE *f) {
  fseek(f, 25, SEEK_CUR);
  int i;
  for (i = 0; i < 64; i++) {
    luma_quantizer[i] = fgetc(f);
  }

  fseek(f, 5, SEEK_CUR);
  for (i = 0; i < 64; i++) {
    chroma_quantizer[i] = fgetc(f);
  }
  return 0;
}

int recoverhuff(huff_code *hc, FILE *f) {

  fseek(f, 2, SEEK_CUR);
  int length;
  length = (fgetc(f) << 8);
  length |= fgetc(f);
  length -= 19;
  fseek(f, 1, SEEK_CUR);

  int i;
  for (i = 0; i < 32; i++) {
    hc->code_len_freq[i] = 0;
  }
  hc->count = 0;
  for (i = 1; i <= 16; i++) {
    hc->code_len_freq[i] = fgetc(f);
    hc->count += hc->code_len_freq[i];
  }

  for (i = 0; i < 256; i++) {
    hc->sym_sorted[i] = -1;
  }
  for (i = 0; i < length; i++) {
    hc->sym_sorted[i] = fgetc(f);
  }

  for (i = 0; i < 256; i++)
    hc->sym_code_len[i] = 0;
  int k = 0;
  int j;
  for (i = 1; i <= 16; i++)
    for (j = 1; j <= hc->code_len_freq[i]; j++)
      hc->sym_code_len[hc->sym_sorted[k++]] = i;
  hc->sym_code_len[hc->sym_sorted[k]] = 0;

  // 分配码符号
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

  //提取有用码字
  k = 0;
  hc->code = (int *)malloc(hc->count * sizeof(int));
  hc->len = (int *)malloc(hc->count * sizeof(int));
  hc->sym = (int *)malloc(hc->count * sizeof(int));
  for (i = 0; i < 256; i++) {
    if (hc->sym_code[i] != -1 && hc->sym_code_len[i] > 0) {
      hc->code[k] = hc->sym_code[i];
      hc->len[k] = hc->sym_code_len[i];
      hc->sym[k] = i;
      k++;
    }
  }

  //生成huffman码树
  hc->root = (tree_root *)malloc(sizeof(tree_root));
  gene_tree(hc->root, hc->code, hc->len, hc->sym, hc->count);

  return 0;
}

int recoversize(jpeg_data *jpg, FILE *f) {
  fseek(f, 5, SEEK_CUR);
  jpg->width = (fgetc(f) << 8);
  jpg->width |= fgetc(f);
  jpg->height = (fgetc(f) << 8);
  jpg->height |= getc(f);
  jpg->num_pixel = jpg->width * jpg->height;
  return 0;
}

int alloc_jpeg_data(jpeg_data *data) {
  data->red = (int *)malloc(data->num_pixel * sizeof(int));
  data->green = (int *)malloc(data->num_pixel * sizeof(int));
  data->blue = (int *)malloc(data->num_pixel * sizeof(int));
  data->rgb = (unsigned int *)malloc(data->num_pixel * sizeof(unsigned int));

  data->y = (int *)malloc(data->num_pixel * sizeof(int));
  data->cb = (int *)malloc(data->num_pixel * sizeof(int));
  data->cr = (int *)malloc(data->num_pixel * sizeof(int));

  data->cb_sub = (int *)malloc(data->num_pixel / 4 * sizeof(int));
  data->cr_sub = (int *)malloc(data->num_pixel / 4 * sizeof(int));

  data->dct_y = (double *)malloc(data->num_pixel * sizeof(double));
  data->dct_cb = (double *)malloc(data->num_pixel / 4 * sizeof(double));
  data->dct_cr = (double *)malloc(data->num_pixel / 4 * sizeof(double));

  data->dct_y_quant = (int *)malloc(data->num_pixel * sizeof(int));
  data->dct_cb_quant = (int *)malloc(data->num_pixel / 4 * sizeof(int));
  data->dct_cr_quant = (int *)malloc(data->num_pixel / 4 * sizeof(int));

  if (data->red == NULL || data->green == NULL || data->blue == NULL ||
      data->y == NULL || data->cb == NULL || data->cr == NULL ||
      data->cb_sub == NULL || data->cr_sub == NULL || data->dct_y == NULL ||
      data->dct_cb == NULL || data->dct_cr == NULL ||
      data->dct_y_quant == NULL || data->dct_cb_quant == NULL ||
      data->dct_cr_quant == NULL) {
    fprintf(stderr, "Could not allocate enough memory for image processing\n");
    return -1;
  }
  return 0;
}

void absolute_dc(int num_pixel, int dct_quant[]) {
  int i;
  for (i = 64; i < num_pixel; i += 64) {
    dct_quant[i] += dct_quant[i - 64];
  }
}

void izigzag(int num_pixel, int dct[]) {
  int i, j;
  int tmp[64];
  for (i = 0; i < num_pixel; i += 64) {
    for (j = 0; j < 64; j++) {

      tmp[j] = dct[i + j];
    }
    for (j = 0; j < 64; j++) {
      dct[i + scan_order[j]] = tmp[j];
    }
  }
}

void iquantize(int num_pixel, double out[], int in[], int quantizer[]) {
  int i;
  for (i = 0; i < num_pixel; i++) {
    out[i] = in[i] * quantizer[i % 64];
  }
}

void idct_block(int gap, double *in, int *out) {

  float32x4_t C10 = {1.0000, 0.9808, 0.9239, 0.8315};
  float32x4_t C11 = {1.0000, 0.8315, 0.3827, -0.1951};
  float32x4_t C12 = {1.0000, 0.5556, -0.3827, -0.9808};
  float32x4_t C13 = {1.0000, 0.1951, -0.9239, -0.5556};
  float32x4_t C14 = {1.0000, -0.1951, -0.9239, 0.5556};
  float32x4_t C15 = {1.0000, -0.5556, -0.3827, 0.9808};
  float32x4_t C16 = {1.0000, -0.8315, 0.3827, 0.1951};
  float32x4_t C17 = {1.0000, -0.9808, 0.9239, -0.8315};

  float32x4_t C20 = {0.7071, 0.5556, 0.3827, 0.1951};
  float32x4_t C21 = {-0.7071, -0.9808, -0.9239, -0.5556};
  float32x4_t C22 = {-0.7071, 0.1951, 0.9239, 0.8315};
  float32x4_t C23 = {0.7071, 0.8315, -0.3827, -0.9808};
  float32x4_t C24 = {0.7071, -0.8315, -0.3827, 0.9808};
  float32x4_t C25 = {-0.7071, -0.1951, 0.9239, -0.8315};
  float32x4_t C26 = {-0.7071, 0.9808, -0.9239, 0.5556};
  float32x4_t C27 = {0.7071, -0.5556, 0.3827, -0.1951};

  int i, j;
  float tmp[8][8];
  float v1[4];
  float v2[4];

  for (i = 0; i < 8; i++) {
    float32x4_t X1 = {
        (float)in[i * 8 + 0], (float)in[i * 8 + 1],
        (float)in[i * 8 + 2], (float)in[i * 8 + 3]};
    float32x4_t X2 = {
        (float)in[i * 8 + 4], (float)in[i * 8 + 5],
        (float)in[i * 8 + 6], (float)in[i * 8 + 7]};
    float32x4_t R1 = vmovq_n_f32(0);
    float32x4_t R2 = vmovq_n_f32(0);

    R1 = vfmaq_laneq_f32(R1, C10, X1, 0);
    R1 = vfmaq_laneq_f32(R1, C11, X1, 1);
    R1 = vfmaq_laneq_f32(R1, C12, X1, 2);
    R1 = vfmaq_laneq_f32(R1, C13, X1, 3);

    R1 = vfmaq_laneq_f32(R1, C14, X2, 0);
    R1 = vfmaq_laneq_f32(R1, C15, X2, 1);
    R1 = vfmaq_laneq_f32(R1, C16, X2, 2);
    R1 = vfmaq_laneq_f32(R1, C17, X2, 3);

    R2 = vfmaq_laneq_f32(R2, C20, X1, 0);
    R2 = vfmaq_laneq_f32(R2, C21, X1, 1);
    R2 = vfmaq_laneq_f32(R2, C22, X1, 2);
    R2 = vfmaq_laneq_f32(R2, C23, X1, 3);

    R2 = vfmaq_laneq_f32(R2, C24, X2, 0);
    R2 = vfmaq_laneq_f32(R2, C25, X2, 1);
    R2 = vfmaq_laneq_f32(R2, C26, X2, 2);
    R2 = vfmaq_laneq_f32(R2, C27, X2, 3);

    vst1q_f32(v1, R1);
    vst1q_f32(v2, R2);
    for (j = 0; j < 4; j++) {
      tmp[i][j] = v1[j];
      tmp[i][j + 4] = v2[j];
    }
  }

  for (j = 0; j < 8; j++) {
    float32x4_t X1 = (float32x4_t){tmp[0][j], tmp[1][j], tmp[2][j], tmp[3][j]};
    float32x4_t X2 = (float32x4_t){tmp[4][j], tmp[5][j], tmp[6][j], tmp[7][j]};
    float32x4_t R1 = vmovq_n_f32(0);
    float32x4_t R2 = vmovq_n_f32(0);

    R1 = vfmaq_laneq_f32(R1, C10, X1, 0);
    R1 = vfmaq_laneq_f32(R1, C11, X1, 1);
    R1 = vfmaq_laneq_f32(R1, C12, X1, 2);
    R1 = vfmaq_laneq_f32(R1, C13, X1, 3);

    R1 = vfmaq_laneq_f32(R1, C14, X2, 0);
    R1 = vfmaq_laneq_f32(R1, C15, X2, 1);
    R1 = vfmaq_laneq_f32(R1, C16, X2, 2);
    R1 = vfmaq_laneq_f32(R1, C17, X2, 3);

    R2 = vfmaq_laneq_f32(R2, C20, X1, 0);
    R2 = vfmaq_laneq_f32(R2, C21, X1, 1);
    R2 = vfmaq_laneq_f32(R2, C22, X1, 2);
    R2 = vfmaq_laneq_f32(R2, C23, X1, 3);

    R2 = vfmaq_laneq_f32(R2, C24, X2, 0);
    R2 = vfmaq_laneq_f32(R2, C25, X2, 1);
    R2 = vfmaq_laneq_f32(R2, C26, X2, 2);
    R2 = vfmaq_laneq_f32(R2, C27, X2, 3);

    vst1q_f32(v1, R1);
    vst1q_f32(v2, R2);
    for (i = 0; i < 4; i++) {
      out[i * gap + j] = v1[i] / 4;
      out[(i + 4) * gap + j] = v2[i] / 4;
    }
  }

  // freq *= M_SQRT1_2;
  for (i = 0; i < 8; i++) {
    out[i] *= M_SQRT1_2;
    out[i * gap] *= M_SQRT1_2;
  }
}

void idct(int blocks_horiz, int blocks_vert, double *in, int *out) {
  int h, v;
  for (v = 0; v < blocks_vert; v++)
    for (h = 0; h < blocks_horiz; h++)
      idct_block(8 * blocks_horiz, in + (v * blocks_horiz + h) * 64,
                 out + v * blocks_horiz * 64 + h * 8);
}

void recover_chroma(jpeg_data *data) {
  int h, w;
  for (h = 0; h < data->height / 2; h++) {
    for (w = 0; w < data->width / 2; w++) {
      int i = 2 * h * data->width + 2 * w;
      data->cb[i] = data->cb_sub[h * data->width / 2 + w];
      data->cb[i + 1] = data->cb_sub[h * data->width / 2 + w];
      data->cb[i + data->width] = data->cb_sub[h * data->width / 2 + w];
      data->cb[i + data->width + 1] = data->cb_sub[h * data->width / 2 + w];

      data->cr[i] = data->cr_sub[h * data->width / 2 + w];
      data->cr[i + 1] = data->cr_sub[h * data->width / 2 + w];
      data->cr[i + data->width] = data->cr_sub[h * data->width / 2 + w];
      data->cr[i + data->width + 1] = data->cr_sub[h * data->width / 2 + w];
    }
  }
}

void yuyv_to_rgb(jpeg_data *data) {
  int i;
  for (i = 0; i < data->num_pixel; i++) {
    data->blue[i] = 1.164 * (data->y[i] - 16) + 2.018 * (data->cb[i] - 128);
    data->green[i] = 1.164 * (data->y[i] - 16) - 0.813 * (data->cr[i] - 128) -
                     0.391 * (data->cb[i] - 128);
    data->red[i] = 1.164 * (data->y[i] - 16) + 1.596 * (data->cb[i] - 128);
    data->rgb[i] = CLIP(data->red[i], 0, 255);
    data->rgb[i] <<= 8;
    data->rgb[i] |= CLIP(data->green[i], 0, 255);
    data->rgb[i] <<= 8;
    data->rgb[i] |= CLIP(data->blue[i], 0, 255);
  }
}

int decode(jpeg_data *jpg) {
  printf("*****************************Decoding*****************************\n");
  FILE *f = fopen("out.jpg", "r");
  int i, k;
  unsigned char c;
 //Read DQT, DHT, DHT
  printf("Reading DQT, DHT, DHT                                     ");
  fflush(stdout);
  timer();
  recoverquality(f);
  huff_code *hc = &jpg->luma_dc;
  recoverhuff(hc, f);
  hc = &jpg->luma_ac;
  recoverhuff(hc, f);
  hc = &jpg->chroma_dc;
  recoverhuff(hc, f);
  hc = &jpg->chroma_ac;
  recoverhuff(hc, f);
  recoversize(jpg, f);
  printf("%10.3f ms\n", timer());
  // alloc_jpeg_data(jpg);

  fseek(f, 10, SEEK_CUR);
  fseek(f, 10, SEEK_CUR);

  timer();
  printf("Reading the bitstream                                     ");
  fflush(stdout);
  read_coefficients(f, jpg->num_pixel, jpg->dct_y_quant, &jpg->luma_dc,
                    &jpg->luma_ac);

  if (fgetc(f) == 0xFF) {
    fseek(f, 9, SEEK_CUR);
  } else {
    fseek(f, 8, SEEK_CUR);
  }
  read_coefficients(f, jpg->num_pixel / 4, jpg->dct_cb_quant, &jpg->chroma_dc,
                    &jpg->chroma_ac);

  if (fgetc(f) == 0xFF) {
    fseek(f, 9, SEEK_CUR);
  } else {
    fseek(f, 8, SEEK_CUR);
  }
  read_coefficients(f, jpg->num_pixel / 4, jpg->dct_cr_quant, &jpg->chroma_dc,
                    &jpg->chroma_ac);

  printf("%10.3f ms\n", timer());

  //printf("%#X\n", fgetc(f));
  // printf("%#X\n", fgetc(f));

  timer();
  printf("Turning the differential values to absolute values        ");
  fflush(stdout);
  absolute_dc(jpg->num_pixel, jpg->dct_y_quant);
  absolute_dc(jpg->num_pixel / 4, jpg->dct_cb_quant);
  absolute_dc(jpg->num_pixel / 4, jpg->dct_cr_quant);
  printf("%10.3f ms\n", timer());

  timer();
  printf("Reordering coefficients (inverse zig-zag)                 ");
  fflush(stdout);
  izigzag(jpg->num_pixel, jpg->dct_y_quant);

  izigzag(jpg->num_pixel / 4, jpg->dct_cb_quant);
  izigzag(jpg->num_pixel / 4, jpg->dct_cr_quant);
  printf("%10.3f ms\n", timer());


  timer();
  printf("Inversing quantization                                    ");
  fflush(stdout);
  iquantize(jpg->num_pixel, jpg->dct_y, jpg->dct_y_quant, luma_quantizer);
  iquantize(jpg->num_pixel / 4, jpg->dct_cb, jpg->dct_cb_quant,
            chroma_quantizer);
  iquantize(jpg->num_pixel / 4, jpg->dct_cr, jpg->dct_cr_quant,
            chroma_quantizer);
  printf("%10.3f ms\n", timer());


  timer();
  printf("IDCT                                                      ");
  fflush(stdout);
  idct(jpg->width / 8, jpg->height / 8, jpg->dct_y, jpg->y);
  idct(jpg->width / 16, jpg->height / 16, jpg->dct_cb, jpg->cb_sub);
  idct(jpg->width / 16, jpg->height / 16, jpg->dct_cr, jpg->cr_sub);
  printf("%10.3f ms\n", timer());


  

  timer();
  printf("Restoring the chroma value                                ");
  fflush(stdout);
  recover_chroma(jpg);
  printf("%10.3f ms\n", timer());



  timer();
  printf("YCbCr to RGB                                              ");
  fflush(stdout);
  yuyv_to_rgb(jpg);
  printf("%10.3f ms\n", timer());


  // for (i = 0; i < 100000; i++) {
  // if (i % 8 == 0) {
  // printf("\n");
  //}
  // printf("%d ", jpg.y[i]);
  //}

  return 0;
}
