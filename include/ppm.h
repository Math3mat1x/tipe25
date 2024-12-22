#include <stdio.h>
#include <stdlib.h>

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

Image* ppm_open(char* filename);
Image* ppm_create(int width, int height);
void   ppm_free(Image* im);
int    ppm_save(Image* im, char* filename);
Image* ppm_convolution(Image* im, int** kernel, int size);
