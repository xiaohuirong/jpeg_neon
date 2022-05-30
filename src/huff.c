#include "huff.h"

int gene_tree(tree_root *root, int *codes, int *code_lens, int *code_syms,
              int sum_num);
int gene_branch(tree_root *root, int current_code, int current_code_len,
                int current_sym);
node *gene_node(tree_root *root, int current_code, int current_code_len);

int huffdecoder(tree_root *root, int bit);

int gene_tree(tree_root *root, int *codes, int *code_lens, int *code_syms,
              int sum_num) {
  int i;
  root->used_count = 0;
  root->isbegin = 0;
  for (i = 0; i < sum_num; i++) {
    gene_branch(root, codes[i], code_lens[i], code_syms[i]);
  }
  return 0;
}
int gene_branch(tree_root *root, int current_code, int current_code_len,
                int current_sym) {
  int tmp_code = current_code;
  int tmp_len = current_code_len;
  int bit;
  node *last_node = gene_node(root, tmp_code, tmp_len);
  last_node->isend = 1;
  last_node->code_sym = current_sym;
  bit = (tmp_code & 0x01);
  tmp_code >>= 1;
  tmp_len--;
  if (tmp_len == 0) {
    if (bit == 0) {
      root->left = last_node;
    } else {
      root->right = last_node;
    }
    return 0;
  }

  while (tmp_len) {
    node *pre_node = gene_node(root, tmp_code, tmp_len);
    if (bit == 0) {
      pre_node->left = last_node;
    } else {
      pre_node->right = last_node;
    }
    if (pre_node->ispublic) {
      break;
    }

    bit = (tmp_code & 0x01);
    tmp_len--;
    tmp_code >>= 1;
    if (tmp_len == 0) {
      if (bit == 0) {
        root->left = pre_node;
      } else {
        root->right = pre_node;
      }
      return 0;
    }
    last_node = pre_node;
  }
  return 0;
}

node *gene_node(tree_root *root, int current_code, int current_code_len) {
  int i;
  for (i = 0; i < root->used_count; i++) {
    if (current_code == root->used_codes[i] &&
        current_code_len == root->used_codes_len[i]) {

      root->used_node[i]->ispublic = 1;
      return root->used_node[i];
    }
  }

  root->used_count++;
  root->used_codes[i] = current_code;
  root->used_codes_len[i] = current_code_len;
  root->used_node[i] = (node *)malloc(sizeof(node));
  root->used_node[i]->isend = 0;
  root->used_node[i]->code = current_code;
  root->used_node[i]->code_len = current_code_len;
  root->used_node[i]->ispublic = 0;
  root->used_node[i]->code_sym = -1;
  return root->used_node[i];
}

int huffdecoder(tree_root *root, int bit) {
  node *p;
  if (root->isbegin == 0) {
    if (bit == 0) {
      p = root->left;
      root->decode = p;
    } else {
      p = root->right;
      root->decode = p;
    }
    root->isbegin = 1;
  }

  else {
    if (bit == 0) {

      p = root->decode->left;
      root->decode = p;

    } else {
      p = root->decode->right;
      root->decode = p;
    }
  }

  if (p->isend == 1) {
    root->isbegin = 0;
    return p->code_sym;
  } else {
    return -1;
  }
}

int read_bits(FILE *f, unsigned char *buffer, int *left, int bit_num) {
  int i;
  int bit;
  int ret = 0;
  if (bit_num == 0) {
    return 0;
  }
  for (i = 0; i < bit_num; i++) {
    bit = (((*buffer) >> 7) & 0x01);
    // printf("%d ", bit);
    ret <<= 1;
    ret |= bit;
    (*buffer) <<= 1;
    (*left)--;
    if ((*left) == 0) {
      (*buffer) = fgetc(f);
      // printf("buffer:%#X\n", *buffer);
      if ((*buffer) == 0xff) {
        fgetc(f);
      }
      // printf("bits:");
      (*left) = 8;
    }
  }
  return ret;
}

int read_sym(FILE *f, huff_code *huff, unsigned char *buffer, int *left) {
  int ret = -1;
  int bit;
  do {
    bit = (((*buffer) >> 7) & 0x01);
    // printf("%d ", bit);
    ret = huffdecoder(huff->root, bit);

    (*buffer) <<= 1;
    (*left)--;
    if ((*left) == 0) {
      (*buffer) = fgetc(f);
      // printf("buffer:%#X\n", *buffer);
      if ((*buffer) == 0xff) {
        fgetc(f);
      }

      // printf("bits:");
      (*left) = 8;
    }
  } while (ret == -1);
  // printf("finish\n");

  return ret;
}

int get_dc(FILE *f, huff_code *huff_dc, unsigned char *buffer, int *left) {

  int sym = read_sym(f, huff_dc, buffer, left);
  int bit_num = sym;
  if (bit_num == 0) {
    return 0;
  }

  int val = read_bits(f, buffer, left, bit_num);
  int flag = ((val >> (bit_num - 1)) & 0x01);
  if (flag == 1) {
    return val;
  } else {
    return -(pow(2, bit_num) - 1 - val);
  }
}

int get_ac(FILE *f, huff_code *huff_ac, unsigned char *buffer, int *left,
           int *val_ac) {
  int sym = read_sym(f, huff_ac, buffer, left);
  // printf("sym=%d ", sym);
  int i = 0;
  int pixel_num = 0;

  if (sym == 0x00) {
    return pixel_num;
  }

  else if (sym == 0xF0) {
    for (i = 0; i < 16; i++) {
      val_ac[i] = 0;
      pixel_num++;
    }
    return pixel_num;
  }

  int bit_num = (sym & 0x0f);
  // printf("bit_num:%d ", bit_num);
  int zero_num = ((sym >> 4) & 0x0f);
  for (i = 0; i < zero_num; i++) {
    val_ac[i] = 0;
    pixel_num++;
  }

  int val = read_bits(f, buffer, left, bit_num);
  int flag = (val >> (bit_num - 1) & 0x01);
  if (flag == 1) {
    val_ac[i] = val;
    pixel_num++;
  } else {
    val_ac[i] = -(pow(2, bit_num) - 1 - val);
    pixel_num++;
  }

  return pixel_num;
}

// int get_ac(FILE *f, huff_code *huff_ac, unsigned char *buffer, int *left)
// {}

// int huffdecoder(tree_root *root, int bit);

int read_coefficients(FILE *f, int num_pixel, int dct_quant[],
                      huff_code *huff_dc, huff_code *huff_ac) {
  int read = 0;
  int i;
  unsigned char buffer;
  buffer = fgetc(f);
  // printf("\nbuffer:%#X\n", buffer);
  int left = 8;
  int dc_val;
  int ac_val[100];
  int ac_len;
  while (read < num_pixel) {
    // printf("read:%d\n", read);

    if (read % 64 == 0) {
      dc_val = get_dc(f, huff_dc, &buffer, &left);
      // printf("dc_val:%d\n", dc_val);
      dct_quant[read] = dc_val;
      read++;
    }

    else {
      ac_len = get_ac(f, huff_ac, &buffer, &left, ac_val);
      if (ac_len == 0) {
        while (read % 64 != 0) {
          dct_quant[read] = 0;
          read++;
        }
      } else {

        for (i = 0; i < ac_len; i++) {
          dct_quant[read] = ac_val[i];
          read++;
        }
      }
    }
  }

  return 0;
}

int test(void) {
  int codes[6] = {0x11, 0x10, 0x00, 0x010, 0x0110, 0x0111};
  int codes2[6] = {3, 2, 0, 2, 6, 7};
  int lens[6] = {2, 2, 2, 3, 4, 4};
  int syms[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

  tree_root root;
  gene_tree(&root, codes2, lens, syms, 6);
  huffdecoder(&root, 0);
  huffdecoder(&root, 1);
  huffdecoder(&root, 1);
  int c = huffdecoder(&root, 1);
  printf("%#X\n", c);
  huffdecoder(&root, 1);
  c = huffdecoder(&root, 0);
  printf("%#X\n", c);

  return 0;
}
