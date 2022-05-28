#include "jpeg_encoder_neon.h"
#include "getimage.h"


int main(void){
  int fd;
  const char device[] = "/dev/video0";
  unsigned char *p[4];
  unsigned int size[4];
  unsigned width = 512;
  unsigned height = 512;
  unsigned quality = 100;
  __u32 format = V4L2_PIX_FMT_YUYV;
  struct v4l2_buffer readbuffer;

  fd = initcamera(device, p, size, format, width, height);
  getimage(fd, &readbuffer);
  encode(p[0], width, height, quality);
  getfinish(fd, &readbuffer);
  close(fd);

  return 0;
}

