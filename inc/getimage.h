#ifndef __GETIMAGE_H
#define __GETIMAGE_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h> //命令在此头文件中
#include <string.h> //memset需要
#include <sys/mman.h> //映射相关

int initcamera(unsigned char* device, unsigned char *mptr[4], unsigned int size[4], __u32 format, unsigned int width, unsigned int height);
int getimage(int fd, struct v4l2_buffer* readbuffer);
int getfinish(int fd, struct v4l2_buffer* readbuffer);


#endif
