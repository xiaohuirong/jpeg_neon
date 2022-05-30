#ifndef __GETIMAGE_H
#define __GETIMAGE_H

#include <fcntl.h>
#include <linux/videodev2.h> //命令在此头文件中
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //memset需要
#include <sys/ioctl.h>
#include <sys/mman.h> //映射相关
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int initcamera(const char *device, unsigned char *mptr[4], unsigned int size[4],
               __u32 format, unsigned int width, unsigned int height);
int getimage(int fd, struct v4l2_buffer *readbuffer);
int getfinish(int fd, struct v4l2_buffer *readbuffer);
int closecamera(int fd);

#endif
