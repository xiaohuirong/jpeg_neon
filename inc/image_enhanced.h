#ifndef __IMAGE_ENHANCED_H__
#define __IMAGE_ENHANCED_H__

#include <stdio.h>
#include "jpeg_encoder.h"
/*对外接口*/
int YCbCr2RGB(jpeg_data* data);
int image_enhanced(jpeg_data* data);

#endif