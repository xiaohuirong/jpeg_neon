#ifndef __SHOW_H
#define __SHOW_H
#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int initlcd(void);
void show(unsigned int *rgb, int w, int h);

#endif
