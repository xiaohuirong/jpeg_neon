//#include "jpeg_encoder_neon.h"
#include "getimage.h"
#include "jpeg.h"
#include "show.h"

int main(void) {

  int fd;
  const char device[] = "/dev/video0";
  unsigned char *p[4];
  unsigned int size[4];
  unsigned width = 640;
  unsigned height = 480;
  unsigned quality = 100;
  __u32 format = V4L2_PIX_FMT_YUYV;
  struct v4l2_buffer readbuffer;
  jpeg_data jpg;
  jpg.width = width;
  jpg.height = height;
  jpg.num_pixel = width * height;

  alloc_jpeg_data(&jpg);

  initlcd();
  fd = initcamera(device, p, size, format, width, height);
  int i = 1;
  while (1) {
    getimage(fd, &readbuffer);
    encode(p[0], &jpg, width, height, quality);
    getfinish(fd, &readbuffer);
    show(jpg.rgb, width, jpg.height);
     //i--;
  }
  closecamera(fd);

  jpeg_data readjpg;
  readjpg.width = width;
  readjpg.height = height;
  readjpg.num_pixel = width * height;

  alloc_jpeg_data(&readjpg);

  decode(&readjpg);

  show(jpg.rgb, width, jpg.height);
  // int i;
  // for (i = 0; i < 512; i++) {
  // if (i % 8 == 0) {
  // printf("\n");
  //}
  // printf("%#X ", readjpg.rgb[i]);
  //}

  return 0;
}
