#include "ppm.h"
#include "csvg.h"
#include <assert.h>
#include <math.h>

#define PI 3.141592
#define EPSILON 1E-6

typedef struct ridge {
  float angle;
  float coherence;
} Ridge;

typedef struct fingerprint {
  int width;
  int height;
  Ridge** ridges;
} Fingerprint;

float squared_average_gradient(Image* grad_x, Image* grad_y, int block_size, int x, int y, int* directions) {
  // Check if the block would go out of bounds
  if (((x + block_size) > (grad_x -> width)) || ((y + block_size) > (grad_y -> height))) {
    return EPSILON;
  }

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

void ridge_valey_orientation(Image* grad_x, Image* grad_y, int block_size, int x, int y, float* angle, float* coherence) {
  int* dir1 = malloc(sizeof(int) * 3);
  int* dir2 = malloc(sizeof(int) * 3);
  int* dir3 = malloc(sizeof(int) * 3);
  dir1[0] = 1;
  dir1[1] = 0;
  dir1[2] = 0;
  
  dir2[0] = 0;
  dir2[1] = 1;
  dir2[2] = 0;
  
  dir3[0] = 0;
  dir3[1] = 0;
  dir3[2] = 1;

  float gxx = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir1);
  float gxy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir2);
  float gyy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir3);

  // Handle division by zero
  if (fabs(gxy) < EPSILON) {
    *angle = 0.0;
  } else {
    *angle = atan2((gxx - gyy), (2 * gxy)) / 2;
  }
  
  // Handle division by zero for coherence
  if (fabs(gxx + gyy) < EPSILON) {
    *coherence = 0.0;
  } else {
    *coherence = sqrt((gxx - gyy) * (gxx - gyy) + 4 * gxy * gxy) / (gxx + gyy);
  }

  free(dir1);
  free(dir2);
  free(dir3);
}

Fingerprint* create_fingerprint(int width, int height) {
  Fingerprint* res = malloc(sizeof(Fingerprint));
  res -> width = width;
  res -> height = height;
  res -> ridges = malloc(sizeof(Ridge*) * height);

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

void normalize_coherence(Fingerprint* fp) {
  float max = 0;
  for (int i = 0; i < (fp -> width); i++) {
    for (int j = 0; j < (fp -> height); j++) {
      float tmp = (fp -> ridges)[j][i].coherence;
      if (tmp > max) {
        max = tmp;
      }
    }
  }

  // Prevent division by zero
  if (fabs(max) < EPSILON) {
    return;
  }

  for (int i = 0; i < (fp -> width); i++) {
    for (int j = 0; j < (fp -> height); j++) {
      float tmp = (fp -> ridges)[j][i].coherence;
      (fp -> ridges)[j][i].coherence = tmp / max;
    }
  }
}

void generate_sobel_kernels(int size, int** sobel_x, int** sobel_y) {
  // TODO : find a general formula
  *sobel_x = malloc(sizeof(int) * size * size);
  *sobel_y = malloc(sizeof(int) * size * size);
  
  if (size == 3) {
    int x_values[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
    int y_values[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
    for (int i = 0; i < 9; i++) {
      (*sobel_x)[i] = x_values[i];
      (*sobel_y)[i] = y_values[i];
    }
  } else if (size == 5) {
    int x_values[25] = {
      2, 1, 0, -1, -2,
      3, 2, 0, -2, -3,
      4, 3, 0, -3, -4,
      3, 2, 0, -2, -3,
      2, 1, 0, -1, -2
    };
    int y_values[25] = {
      2, 3, 4, 3, 2,
      1, 2, 3, 2, 1,
      0, 0, 0, 0, 0,
      -1, -2, -3, -2, -1,
      -2, -3, -4, -3, -2
    };
    for (int i = 0; i < 25; i++) {
      (*sobel_x)[i] = x_values[i];
      (*sobel_y)[i] = y_values[i];
    }
  } else if (size == 7) {
    int x_values[49] = {
      3, 2, 1, 0, -1, -2, -3,
      4, 3, 2, 0, -2, -3, -4,
      5, 4, 3, 0, -3, -4, -5,
      6, 5, 4, 0, -4, -5, -6,
      5, 4, 3, 0, -3, -4, -5,
      4, 3, 2, 0, -2, -3, -4,
      3, 2, 1, 0, -1, -2, -3
    };
    int y_values[49] = {
      3, 4, 5, 6, 5, 4, 3,
      2, 3, 4, 5, 4, 3, 2,
      1, 2, 3, 4, 3, 2, 1,
      0, 0, 0, 0, 0, 0, 0,
      -1, -2, -3, -4, -3, -2, -1,
      -2, -3, -4, -5, -4, -3, -2,
      -3, -4, -5, -6, -5, -4, -3
    };
    for (int i = 0; i < 49; i++) {
      (*sobel_x)[i] = x_values[i];
      (*sobel_y)[i] = y_values[i];
    }
  }
}

// Calculate the ridge frequency in a local region
// TESTED
float calculate_local_ridge_frequency(Image* im, int x, int y, float orientation, int window_size) {
  // Ensure the window fits within the image
  if (x < window_size/2 || y < window_size/2 || 
      x + window_size/2 >= im->width || y + window_size/2 >= im->height) {
    return 0.0;
  }
  
  float cos_theta = cos(orientation); // is orientation in gradients?
  float sin_theta = sin(orientation);
  
  // Sample points perpendicular to ridge direction
  int num_samples = window_size;
  int* gray_levels = malloc(sizeof(int) * num_samples);
  
  // Center of the window
  int center_x = x;
  int center_y = y;
  
  // Sample along the perpendicular direction
  for (int i = 0; i < num_samples; i++) {
    // Calculate offset from center (-window_size/2 to +window_size/2)
    float offset = i - window_size/2;
    
    // Note: perpendicular direction is (-sin_theta, cos_theta)
    // direction is clockwise
    int sample_x = (int)(center_x - offset * sin_theta);
    int sample_y = (int)(center_y + offset * cos_theta);
    
    // Ensure the sample point is within the image
    if (sample_x < 0) sample_x = 0;
    if (sample_y < 0) sample_y = 0;
    if (sample_x >= im->width) sample_x = im->width - 1;
    if (sample_y >= im->height) sample_y = im->height - 1;
    
    // Get the gray level at the sample point (using red channel for grayscale)
    gray_levels[i] = im->p[sample_y][sample_x].r;
  }
  
  // Count the number of peaks (ridges) in the sampled data
  int peak_count = 0;
  for (int i = 1; i < num_samples - 1; i++) {
    // A peak is where the value is higher than both neighbors
    if (gray_levels[i] > gray_levels[i-1] && gray_levels[i] > gray_levels[i+1]) {
      peak_count++;
    }
  }
  
  free(gray_levels);
  
  // Calculate frequency (peaks per pixel)
  float frequency = 0.0;
  if (peak_count > 0) {
    frequency = (float)peak_count / window_size;
  }
  
  return frequency;
}

int estimate_ridge_frequency(Image* im) {
  int temp_block_size = 8;
  
  int* sobel_x;
  int* sobel_y;
  generate_sobel_kernels(3, &sobel_x, &sobel_y);
  
  // Apply convolution
  Image* grad_x = ppm_convolution(im, sobel_x, 3);
  Image* grad_y = ppm_convolution(im, sobel_y, 3);
  
  // Sample points across the image to estimate average frequency
  int num_samples = 16;
  float total_frequency = 0.0;
  int valid_samples = 0;
  
  // Sample in a grid pattern
  for (int i = 0; i < num_samples; i++) {
    int x = im->width / num_samples * i + im->width / (2 * num_samples);
    
    for (int j = 0; j < num_samples; j++) {
      int y = im->height / num_samples * j + im->height / (2 * num_samples);
      
      // Calculate local orientation
      float angle, coherence;
      // Use actual pixel coordinates instead of dividing by block_size
      ridge_valey_orientation(grad_x, grad_y, temp_block_size, 
                             x - temp_block_size/2, y - temp_block_size/2, 
                             &angle, &coherence);
      
      // Only use points with high coherence (clear ridge pattern)
      if (coherence > 0.5) {
        // Calculate local frequency with a window size of 24 pixels
        float freq = calculate_local_ridge_frequency(im, x, y, angle, 24);
        
        if (freq > 0.0) {
          total_frequency += freq;
          valid_samples++;
        }
      }
    }
  }
  
  // Clean up
  free(sobel_x);
  free(sobel_y);
  ppm_free(grad_x);
  ppm_free(grad_y);

  float avg_frequency = 0.0;
  if (valid_samples > 0) {
    avg_frequency = total_frequency / valid_samples;
  }

  if ((valid_samples == 0) || (avg_frequency < 0.05)) {
    return 3;
  } else {
      float avg_ridge_width = 1.0 / avg_frequency;

      if (avg_ridge_width > 12) {
        return 7; // Wide ridges - use 7x7 kernel
      } else if (avg_ridge_width > 6) {
        return 5; // Medium ridges - use 5x5 kernel
      } else {
        return 3; // Narrow ridges - use 3x3 kernel
      }
  }
}

Fingerprint* compute_fingerprint(Image* im, int block_size) {
  // Generate the appropriate Sobel kernels
  int* sobel_x;
  int* sobel_y;
  generate_sobel_kernels(block_size, &sobel_x, &sobel_y);
  
  // Apply convolution with the selected kernel size
  Image* grad_x = ppm_convolution(im, sobel_x, block_size);
  Image* grad_y = ppm_convolution(im, sobel_y, block_size);

  // Calculate the number of blocks that fit in the image
  int x_blocks = (im->width - block_size + 1) / block_size;
  int y_blocks = (im->height - block_size + 1) / block_size;
  
  // Ensure we have at least one block
  if (x_blocks < 1) x_blocks = 1;
  if (y_blocks < 1) y_blocks = 1;
  
  // Create fingerprint with the correct dimensions
  Fingerprint* fp = create_fingerprint(x_blocks, y_blocks);

  // Process each block
  for (int i = 0; i < x_blocks; i++) {
    for (int j = 0; j < y_blocks; j++) {
      // Calculate the top-left corner of each block
      int block_x = i * block_size;
      int block_y = j * block_size;
      
      // Calculate orientation for this block
      ridge_valey_orientation(grad_x, grad_y, block_size, block_x, block_y, 
                             &((fp->ridges)[j][i].angle), 
                             &((fp->ridges)[j][i].coherence));
    }
  }

  free(sobel_x);
  free(sobel_y);
  ppm_free(grad_x);
  ppm_free(grad_y);

  normalize_coherence(fp);

  return fp;
}

void draw_svg(Fingerprint* fp, const char* filename) {
  int spacing = 20;
  SVG* svg = svg_init(filename, spacing * (fp -> width), spacing * (fp -> height));

  for (int i = 0; i < (fp -> width); i++) {
    for (int j = 0; j < (fp -> height); j++) {
      int color_value = (int)round(fp->ridges[j][i].coherence * 255);
      unsigned int color = (color_value << 16) | (color_value << 8) | color_value;
      float angle = (fp -> ridges)[j][i].angle;
      svg_line(svg, i * spacing, (j + 1) * spacing, (i + cos(angle)) * spacing, (j + 1 - sin(angle)) * spacing, 10, color);
    }
  }

  svg_close(svg);
}

int main(int argc, char **argv) {
  Image* im = ppm_open(argv[1]);
  int block_size = estimate_ridge_frequency(im);
  printf("Estimated frequency: %d.\n", block_size);
  // TODO : gaussian windowing?
  Fingerprint* fp = compute_fingerprint(im, block_size);
  draw_svg(fp,"fingerprint.svg");
  return 0;
}
