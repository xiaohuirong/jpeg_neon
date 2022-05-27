#ifndef __JPEG_ENCODER_NEON_H
#define __JPEG_ENCODER_NEON_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <arm_neon.h>


void printf1(void);
int encode(char* ycbcr, unsigned int width, unsigned int height, unsigned int quality);


#endif 
