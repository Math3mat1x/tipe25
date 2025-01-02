#include "ppm.h"
#include "csvg.h"
#include <assert.h>
#include <math.h>

#define PI 3.14159265358979323846
#define EPSILON 1E-6

typedef struct ridge {
  float angle;
  int coherence;
} Ridge;

typedef struct fingerprint {
  int width;
  int height;
  Ridge** ridges;
} Fingerprint;

float squared_average_gradient(Image* grad_x, Image* grad_y, int block_size, int x, int y, int* directions) {
  // TODO: this fails with block_size = 1
  assert(((x + block_size) < (grad_x->width)) && ((y + block_size) < (grad_y->height)));

  float res = 0;
  for (int i = 0; i < block_size; i++) {
    for (int j = 0; j < block_size; j++) {
      // Access the red channel for greyscale images
      float tmp_x = (grad_x->p)[y + j][x + i].r;
      float tmp_y = (grad_y->p)[y + j][x + i].r;

      if (directions[0] == 1) { 
        res += tmp_x * tmp_x; // Compute Gxx
      }
      if (directions[1] == 1) {
        res += tmp_x * tmp_y; // Compute Gxy
      }
      if (directions[2] == 1) {
        res += tmp_y * tmp_y; // Compute Gyy
      }
    }
  }

  return res + EPSILON;
}

void ridge_valey_orientation(Image* grad_x, Image* grad_y, int block_size, int x, int y, float* angle, int* coherence) {
  int* dir1 = malloc(sizeof(int) * 3);
  int* dir2 = malloc(sizeof(int) * 3);
  int* dir3 = malloc(sizeof(int) * 3);
  dir1[0] = 1;
  dir2[1] = 1;
  dir3[2] = 1;

  float gxx = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir1);
  float gxy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir2);
  float gyy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir3);

  *angle = atan((gxx - gyy) /  (2 * gxy)) / 2; // TODO : check
  *coherence = sqrt((gxx - gyy) * (gxx - gyy) + 4 * gxy * gxy) / (gxx + gyy);

  free(dir1);
  free(dir2);
  free(dir3);
}

Fingerprint* create_blank_fingerprint(int width, int height) {
  Fingerprint* res = malloc(sizeof(Fingerprint));
  res -> width = width;
  res -> height = height;
  res -> ridges = malloc(sizeof(Ridge) * height);

  for (int j = 0; j < height; j++) {
    (res -> ridges)[j] = malloc(sizeof(Ridge) * width);
  }

  return res;
}

void free_fingerprint(Fingerprint* fp) {

  for (int j = 0; j < (fp -> height); j++) {
    free((fp -> ridges)[j]);
  }
  free(fp -> ridges);
  free(fp);
}

Fingerprint* compute_fingerprint(Image* im, int block_size) {
  // locksize : a divisor of gcd(width, height)

  // compute the gradients using the sobel operator
  int* sobel_x = malloc(sizeof(int) * 9);
  int x_values[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
  for (int i = 0; i < 9; i++) (sobel_x[i] = x_values[i]);

  int* sobel_y = malloc(sizeof(int) * 9);
  int y_values[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
  for (int i = 0; i < 9; i++) (sobel_y[i] = y_values[i]);

  Image* grad_x = ppm_convolution(im, sobel_x, 3);
  Image* grad_y = ppm_convolution(im, sobel_y, 3);

  // initialization of fingerprint
  int x = im -> width / block_size;
  int y = im -> height / block_size;
  Fingerprint* fp = create_blank_fingerprint(x, y);

  for (int i = 0; i < x; i++) {
    for (int j = 0; j < y; j++) {
      ridge_valey_orientation(grad_x, grad_y, block_size, i, j, &((fp -> ridges)[j][i].angle), &((fp -> ridges)[j][i].coherence));
    }
  }

  free(sobel_x);
  free(sobel_y);
  ppm_free(grad_x);
  ppm_free(grad_y);

  return fp;
}

void draw_svg(Fingerprint* fp, const char* filename) {
  int spacing = 20;
  SVG* svg = svg_init(filename, spacing * (fp -> width), spacing * (fp -> height));
  int color = 0x000000;

  for (int i = 0; i < (fp -> width); i++) {
    for (int j = 0; j < (fp -> height); j++) {
      float angle = (fp -> ridges)[j][i].angle;
      printf("%f ", angle);
      svg_line(svg, i * spacing, (j + 1) * spacing, (i + cos(angle)) * spacing, (j + 1 - sin(angle)) * spacing, 1, color);
    }
  }

  svg_close(svg);
}

int main(int argc, char **argv) {
  Image* im = ppm_open(argv[1]);
  Fingerprint* fp = compute_fingerprint(im, 16);
  draw_svg(fp,"fingerprint.svg");
  return 0;
}
