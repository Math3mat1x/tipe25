#include "ppm.h"

Image* ppm_create(int width, int height) {
  Image* im = malloc(sizeof(Image));
  im -> width = width;
  im -> height = height;
  im -> p = malloc(sizeof(Pixel*) * height);
  for (int j = 0; j < height; j++) {
    (im -> p)[j] = malloc(sizeof(Pixel) * width);
  }

  return im;
}

void ppm_free(Image* im) {
  for (int i = 0; i < (im -> height); i++) free((im->p)[i]);

  free(im -> p);
  free(im);
}

Image* ppm_open(char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Error opening file");
        return NULL;
    }

    char buff[16];
    if (fgets(buff, sizeof(buff), f) == NULL || (buff[0] != 'P' || buff[1] != '6')) {
        fprintf(stderr, "Invalid PPM file format\n");
        fclose(f);
        return NULL;
    }

    int width, height, maxval;
    if (fscanf(f, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "Error reading image dimensions\n");
        fclose(f);
        return NULL;
    }

    if (fscanf(f, "%d", &maxval) != 1 || maxval != 255) {
        fprintf(stderr, "Unsupported maxval or error reading maxval\n");
        fclose(f);
        return NULL;
    }
    fgetc(f); // Consommer le saut de ligne après maxval

    Image* im = ppm_create(width, height);
    if (!im) {
        fprintf(stderr, "Error creating image\n");
        fclose(f);
        return NULL;
    }

    for (int j = 0; j < im->height; j++) {
        for (int i = 0; i < im->width; i++) {
            fread(&(im->p[j][i].r), 1, 1, f);
            fread(&(im->p[j][i].g), 1, 1, f);
            fread(&(im->p[j][i].b), 1, 1, f);
        }
    }

    fclose(f);
    return im;
}

int ppm_save(Image* im, char* filename) {
    FILE* f = fopen(filename, "wb");

    // Write PPM header
    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", im->width, im->height);
    fprintf(f, "255\n");

    // Write pixel data
    for (int j = 0; j < im->height; j++) {
        for (int i = 0; i < im->width; i++) {
            Pixel p = im->p[j][i];
            fwrite(&p.r, 1, 1, f);
            fwrite(&p.g, 1, 1, f);
            fwrite(&p.b, 1, 1, f);
        }
    }

    fclose(f);
    return 0; // Success
}

int min(int a, int b) { return a < b ? a : b;}
int max(int a, int b) { return a > b ? a : b;}
  
Image* ppm_convolution(Image* im, int** kernel, int size) {
    if (!im || !kernel || size <= 0 || size > im->width || size > im->height) {
        return NULL; // Invalid inputs
    }

    int new_width = im->width - size + 1;
    int new_height = im->height - size + 1;

    Image* result = ppm_create(new_width, new_height);

    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0;

            for (int j = 0; j < size; j++) {
                for (int i = 0; i < size; i++) {
                    int img_x = x + i;
                    int img_y = y + j;

                    sum_r += kernel[j][i] * im->p[img_y][img_x].r;
                    sum_g += kernel[j][i] * im->p[img_y][img_x].g;
                    sum_b += kernel[j][i] * im->p[img_y][img_x].b;
                }
            }

            result->p[y][x].r = sum_r < 0 ? 0 : min(sum_r, 255);
            result->p[y][x].g = sum_g < 0 ? 0 : min(sum_g, 255);
            result->p[y][x].b = sum_b < 0 ? 0 : min(sum_b, 255);
        }
    }

    return result;
}
