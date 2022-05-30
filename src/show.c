#include "show.h"

int lcd_w = 1366;
int lcd_h = 768;

int initlcd(unsigned int *lcdptr) {
  int lcdfd = open("/dev/fb0", O_RDWR);

  struct fb_var_screeninfo info;
  int lret = ioctl(lcdfd, FBIOGET_VSCREENINFO, &info);

  lcd_w = info.xres;
  lcd_h = info.yres;
  printf("width:%d  height:%d\n", lcd_w, lcd_h);

  lcdptr = (unsigned int *)mmap(NULL, lcd_w * lcd_h * 4, PROT_READ | PROT_WRITE,
                                MAP_SHARED, lcdfd, 0);
  return lcdfd;
}

int show_rgb_data(unsigned char *rgbdata, unsigned int *lcdptr, int width,
                  int height) {
  int i;
  int j;
  unsigned *ptr = lcdptr;
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      memcpy(ptr + j, rgbdata + j * 3, 3);
    }
    printf("i:%d ,j:%d \n", i, j);
    ptr += lcd_w;
    rgbdata += width * 3;
  }
  return 0;
}
