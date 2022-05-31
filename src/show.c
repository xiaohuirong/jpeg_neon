#include "show.h"

unsigned char *fb_mem;
struct fb_var_screeninfo var; //可变参数
struct fb_fix_screeninfo fix; //固定参数

/*画点*/
void show_pixel(int x, int y, int color) {
  unsigned char *show8 = NULL;
  unsigned short *show16 = NULL;
  unsigned long *show32 = NULL;
  int red;
  int green;
  int blue;
  /* 定位到LCD屏上的位置 */
  show8 = fb_mem + y * fix.line_length + x * var.bits_per_pixel / 8;
  show16 = (unsigned short *)show8;
  show32 = (unsigned long *)show8;
  switch (var.bits_per_pixel) {
  case 8:

  {
    *show8 = color;
    break;
  }
  case 16: {
    /* RGB:565 */
    red = (color >> 16) & 0xff;
    green = (color >> 8) & 0xff;
    blue = color & 0xff;
    color = ((red >> 3) << 11) | ((green >> 2) << 6) | (blue >> 3);
    *show16 = color;
    break;
  }
  case 32: {
    *show32 = color;
    break;
  }
  default:
    break;
  }
}

int initlcd(void) {

  int fb;
  fb = open("/dev/fb0", 2);
  if (fb < 0) {
    printf("fb0打开失败!\n");
    return -1;
  }

  /*1. 获取可变参数*/
  ioctl(fb, FBIOGET_VSCREENINFO, &var);
  printf("x=%d\n", var.xres);
  printf("y=%d\n", var.yres);
  printf("bit=%d\n", var.bits_per_pixel);

  /*2. 获取固定参数*/
  ioctl(fb, FBIOGET_FSCREENINFO, &fix);
  printf("line_byte=%d\n", fix.line_length);
  printf("smem_len=%d\n", fix.smem_len);

  /*3. 映射LCD地址*/
  fb_mem = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

  // memset(fb_mem, 0x0, fix.smem_len);

  // int i, j;
  // for (i = 0; i < 100; i++) {
  // for (j = 0; j < var.xres; j++) {
  // show_pixel(j, i, 0xFF3333);
  //}
  //}

  return 0;
}

void show(unsigned int *rgb, int w, int h) {
  // memset(fb_mem, 0x0, fix.smem_len);
  int y, x;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      show_pixel(x, y, rgb[y * w + x]);
    }
  }
}
