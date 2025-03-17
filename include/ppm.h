#ifndef PPM_H
#define PPM_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct pixel {
  int r;
  int g;
  int b;
} Pixel;

typedef struct image {
  int width;
  int height;
  Pixel** p;
} Image;

typedef struct ridge {
  float angle;
  float coherence;
} Ridge;

typedef struct fingerprint {
  int width;
  int height;
  Ridge** ridges;
} Fingerprint;

Image* ppm_open(char* filename);
Image* ppm_create(int width, int height);
void   ppm_free(Image* im);
int    ppm_save(Image* im, char* filename);
Image* ppm_convolution(Image* im, int* kernel, int size);
Image* grey_scale(Image* im);
Image* ppm_normalize(Image* im);

#endif
