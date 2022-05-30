//#include "jpeg_encoder_neon.h"
#include "getimage.h"
#include "jpeg.h"
#include "show.h"

int main(void) {

  int fd;
  const char device[] = "/dev/video7";
  unsigned char *p[4];
  unsigned int size[4];
  unsigned width = 640;
  unsigned height = 480;
  unsigned quality = 100;
  __u32 format = V4L2_PIX_FMT_YUYV;
  struct v4l2_buffer readbuffer;

  fd = initcamera(device, p, size, format, width, height);
  getimage(fd, &readbuffer);
  encode(p[0], width, height, quality);
  getfinish(fd, &readbuffer);
  closecamera(fd);

  jpeg_data readjpg;
  decode(&readjpg);

  unsigned int *lcdptr;
  int lcdfd = initlcd(lcdptr);

  show_rgb_data(readjpg.rgb, lcdptr, 640, 480);

  return 0;
}
