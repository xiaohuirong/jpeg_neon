#include "image_enhanced.h"

//图像的三个颜色通道
#define BLUE 0
#define GREEN 1
#define RED 2
#ifndef SCREEN_XY
#define SCREEN_XY(x, y)                                                        \
  (255 - ((255 - (x)) * (255 - (y)) >> 8)) //将新的图层与原图做滤色混合
// x为原始图像像素值，y为新图层的像素值
#endif

/*函数具体声明*/
int YCbCr2RGB(jpeg_data *data) {
  int i;
  for (i = 0; i < data->num_pixel; i++) {
    data->red[i] = 1.164 * (data->y[i] - 16) + 1.596 * (data->cr[i] - 128);
    data->green[i] = 1.164 * (data->y[i] - 16) - 0.813 * (data->cr[i] - 128) -
                     0.392 * (data->cb[i] - 128);
    data->blue[i] = 1.164 * (data->y[i] - 16) + 2.017 * (data->cb[i] - 128);
    // assert( 0<=data->red[i] && data->red[i]<=255);
    // assert( 0<=data->green[i] && data->green[i]<=255 );
    // assert( 0<=data->blue[i] && data->blue[i]<=255 );
  }
}
int image_enhanced(jpeg_data *data) {
  YCbCr2RGB(data);
  int size = data->num_pixel;              //获取原图像的大小
  u_char r = 0, g = 0, b = 0, g_alpha = 0; //定义参数初始值
  for (int i = 0; i < size; i++)           //
  {
    //将绿色通道反色，与b、g、r通道分别相乘，得到新的图层颜色
    g_alpha = 255 - data->green[i]; //将绿色通道反色
    b = data->blue[i] * g_alpha >> 8;
    g = data->green[i] * g_alpha >> 8;
    r = data->red[i] * g_alpha >> 8;

    //将上个步骤得到的新图层，与原始图做滤色混合，即执行f（a,b）=1-(1-a)*(1-b)的操作
    data->blue[i] = SCREEN_XY(data->blue[i], b); //
    data->green[i] = SCREEN_XY(data->green[i], g);
    data->red[i] = SCREEN_XY(data->red[i], r);
    //如果发现使用一次照度增强后，图片仍然偏暗，再运行一次上述代码
    //如果觉得合适，就只增强一次即可
    g_alpha = 255 - data->green[i]; //将绿色通道反色
    b = data->blue[i] * g_alpha >> 8;
    g = data->green[i] * g_alpha >> 8;
    r = data->red[i] * g_alpha >> 8;

    data->blue[i] = SCREEN_XY(data->blue[i], b); //
    data->green[i] = SCREEN_XY(data->green[i], g);
    data->red[i] = SCREEN_XY(data->red[i], r);
  }
  return 0;
}
/******************************/

