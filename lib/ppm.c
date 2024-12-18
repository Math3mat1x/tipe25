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
  for (int i = 0; i < (im -> height); i++) {
    free((im -> p)[i]);
  }

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
