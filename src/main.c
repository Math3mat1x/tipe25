#include "ppm.h"
#include "csvg.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define PI 3.141592
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

// Calcule la moyenne carrée des gradients dans un bloc
float squared_average_gradient(Image* grad_x, Image* grad_y, int block_size, int x, int y, const int* directions) {
  if (!(((x + block_size) < (grad_x->width)) && ((y + block_size) < (grad_y->height)))) {
    return EPSILON;
  }

  float res = 0;
  for (int i = 0; i < block_size; i++) {
    for (int j = 0; j < block_size; j++) {
      float tmp_x = grad_x->p[y + j][x + i].r;
      float tmp_y = grad_y->p[y + j][x + i].r;

      if (directions[0]) res += tmp_x * tmp_x; // Gxx
      if (directions[1]) res += tmp_x * tmp_y; // Gxy
      if (directions[2]) res += tmp_y * tmp_y; // Gyy
    }
  }

  return res + EPSILON;
}

// Calcule l'orientation et la cohérence des crêtes/valées
void ridge_valey_orientation(Image* grad_x, Image* grad_y, int block_size, int x, int y, float* angle, int* coherence) {
  const int dir1[3] = {1, 0, 0};
  const int dir2[3] = {0, 1, 0};
  const int dir3[3] = {0, 0, 1};

  float gxx = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir1);
  float gxy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir2);
  float gyy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir3);

  *angle = atan2((gxx - gyy), (2 * gxy)) / 2.0f;
  *coherence = sqrtf((gxx - gyy) * (gxx - gyy) + 4 * gxy * gxy) / (gxx + gyy);
}

// Crée une empreinte digitale
Fingerprint* create_fingerprint(int width, int height) {
  Fingerprint* fp = malloc(sizeof(Fingerprint));
  fp->width = width;
  fp->height = height;
  fp->ridges = malloc(height * sizeof(Ridge*));

  for (int j = 0; j < height; j++) {
    fp->ridges[j] = calloc(width, sizeof(Ridge));
  }

  return fp;
}

// Libère une empreinte digitale
void free_fingerprint(Fingerprint* fp) {
  for (int j = 0; j < fp->height; j++) {
    free(fp->ridges[j]);
  }
  free(fp->ridges);
  free(fp);
}

// Normalise la cohérence des crêtes
void normalize_coherence(Fingerprint* fp) {
  float max = 0.0f;

  // Trouver la cohérence maximale
  for (int i = 0; i < fp->width; i++) {
    for (int j = 0; j < fp->height; j++) {
      if (fp->ridges[j][i].coherence > max) {
        max = fp->ridges[j][i].coherence;
      }
    }
  }

  // Normaliser
  for (int i = 0; i < fp->width; i++) {
    for (int j = 0; j < fp->height; j++) {
      fp->ridges[j][i].coherence /= max;
    }
  }
}

// Calcule une empreinte digitale à partir d'une image
Fingerprint* compute_fingerprint(Image* im, int block_size) {
  const int sobel_x[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
  const int sobel_y[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

  Image* grad_x = ppm_convolution(im, sobel_x, 3);
  Image* grad_y = ppm_convolution(im, sobel_y, 3);

  int width = im->width / block_size;
  int height = im->height / block_size;
  Fingerprint* fp = create_fingerprint(width, height);

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      ridge_valey_orientation(grad_x, grad_y, block_size, i * block_size, j * block_size, 
                              &fp->ridges[j][i].angle, &fp->ridges[j][i].coherence);
    }
  }

  ppm_free(grad_x);
  ppm_free(grad_y);
  normalize_coherence(fp);

  return fp;
}

// Dessine les crêtes/valées dans un fichier SVG
void draw_svg(Fingerprint* fp, const char* filename) {
  const int spacing = 20;
  SVG* svg = svg_init(filename, spacing * fp->width, spacing * fp->height);

  for (int i = 0; i < fp->width; i++) {
    for (int j = 0; j < fp->height; j++) {
      int color_value = (int)roundf(fp->ridges[j][i].coherence * 255);
      unsigned int color = (color_value << 16) | (color_value << 8) | color_value;
      float angle = fp->ridges[j][i].angle;

      svg_line(svg, i * spacing, (j + 1) * spacing, 
               (i + cosf(angle)) * spacing, (j + 1 - sinf(angle)) * spacing, 
               10, color);
    }
  }

  svg_close(svg);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input_image>\n", argv[0]);
    return EXIT_FAILURE;
  }

  Image* im = ppm_open(argv[1]);
  if (!im) {
    fprintf(stderr, "Error: Unable to open image file %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  Fingerprint* fp = compute_fingerprint(im, 2);
  draw_svg(fp, "fingerprint.svg");

  free_fingerprint(fp);
  ppm_free(im);

  return EXIT_SUCCESS;
}
