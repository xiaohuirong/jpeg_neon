#ifndef __HUFF_H
#define __HUFF_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct __node {
  int code;
  int code_len;
  int isend;
  int ispublic;
  int code_sym;
  struct __node *parent;
  struct __node *left;
  struct __node *right;
};
typedef struct __node node;

typedef struct __root {
  int used_codes[10000];
  int used_codes_len[10000];
  node *used_node[10000];
  int used_count;
  node *left;
  node *right;
  int isbegin;
  node *decode;
} tree_root;

typedef struct __huff_code {
  int sym_freq[257]; // frequency of occurrence of symbol i
  int code_len[257]; // code length of symbol i
  int next[257];     // index to next symbol in chain of all symbols in current
                     // branch of code tree
  int code_len_freq[32]; // the frequencies of huff-symbols of length i
  int sym_sorted[256];   // the symbols to be encoded
  int sym_code_len[256]; // the huffman code length of symbol i
  int sym_code[256];     // the huffman code of the symbol i
  int count;
  int *code;
  int *len;
  int *sym;
  tree_root *root;
} huff_code;

int gene_tree(tree_root *root, int *codes, int *code_lens, int *code_syms,
              int sum_num);
int huffdecoder(tree_root *root, int bit);

int read_coefficients(FILE *f, int num_pixel, int dct_quant[],
                      huff_code *huff_dc, huff_code *huff_ac);

#endif
