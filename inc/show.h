#ifndef __SHOW_H
#define __SHOW_H
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/videodev2.h> //命令在此头文件中
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //memset需要
#include <sys/ioctl.h>
#include <sys/mman.h> //映射相关
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int initlcd(unsigned int *lcdptr);
int show_rgb_data(unsigned char *rgbdata, unsigned int *lcdptr, int width, int height);


#endif
