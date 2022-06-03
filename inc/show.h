#ifndef __SHOW_H
#define __SHOW_H

#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h> //命令在此头文件中
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



int initlcd(void);
void show(unsigned int *rgb, int w, int h);

#endif
