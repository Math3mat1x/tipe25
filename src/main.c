#include "ppm.h"
#include "csvg.h"
#include "gabor.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#define PI 3.141592
#define EPSILON 1E-6

float squared_average_gradient(Image* grad_x, Image* grad_y, int block_size, int x, int y, int* directions) {
  // Check if the block would go out of bounds
  // (x,y) represents the top left of the moving window
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

  return res / (block_size * block_size + EPSILON);  // Corrected normalization
}


void ridge_valey_orientation(Image* grad_x, Image* grad_y, int block_size, int x, int y, float* angle, float* coherence) {
  int dir1[3] = {1, 0, 0};
  int dir2[3] = {0, 1, 0};
  int dir3[3] = {0, 0, 1};

  float gxx = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir1);
  float gxy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir2);
  float gyy = squared_average_gradient(grad_x, grad_y, block_size, x, y, dir3);

  // Compute orientation angle
  *angle = 0.5 * atan2(2.0 * gxy, gxx - gyy) + PI/2.0;
  
  // Compute coherence using the correct formula
  float numerator = sqrt((gxx - gyy) * (gxx - gyy) + 4 * gxy * gxy);
  float denominator = gxx + gyy;
  
  if (denominator > EPSILON) {
    *coherence = numerator / denominator;
  } else {
    *coherence = 0.0;
  }
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
  // TODO : find a general formula
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


Fingerprint* compute_fingerprint(Image* im, int block_size) {
  // Generate the appropriate Sobel kernels
  int* sobel_x;
  int* sobel_y;
  generate_sobel_kernels(block_size, &sobel_x, &sobel_y);
  
  // Apply convolution with the selected kernel size
  Image* grad_x = ppm_convolution(im, sobel_x, block_size);
  Image* grad_y = ppm_convolution(im, sobel_y, block_size);

  // Calculate the number of blocks in the image dimensions
  int x_blocks = im->width / block_size;
  int y_blocks = im->height / block_size;
  
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
      
      // Make sure the block is within bounds
      if (block_x + block_size <= grad_x->width && block_y + block_size <= grad_y->height) {
        // Calculate orientation for this block
        ridge_valey_orientation(grad_x, grad_y, block_size, block_x, block_y, 
                              &((fp->ridges)[j][i].angle), 
                              &((fp->ridges)[j][i].coherence));
      } else {
        // Set default values for out-of-bounds blocks
        (fp->ridges)[j][i].angle = 0.0;
        (fp->ridges)[j][i].coherence = 0.0;
      }
    }
  }

  // Clean up resources
  free(sobel_x);
  free(sobel_y);
  ppm_free(grad_x);
  ppm_free(grad_y);

  // Normalize the coherence values
  normalize_coherence(fp);

  return fp;
}

void draw_svg(Fingerprint* fp, const char* filename) {
  int spacing = 20;
  SVG* svg = svg_init(filename, spacing * (fp -> width), spacing * (fp -> height));

  for (int i = 0; i < (fp -> width); i++) {
    for (int j = 0; j < (fp -> height); j++) {
      float coherence = (fp -> ridges)[j][i].coherence;
      if (coherence < 0.2) continue;  // Skip low coherence blocks
      
      int color_value = (int)round(coherence * 255);
      unsigned int color = (color_value << 16) | (color_value << 8) | color_value;
      
      float angle = (fp -> ridges)[j][i].angle;
      // Draw line centered at block center
      int center_x = i * spacing + spacing/2;
      int center_y = j * spacing + spacing/2;
      int line_length = spacing/2;
      
      svg_line(svg, 
               center_x - line_length * cos(angle),
               center_y - line_length * sin(angle),
               center_x + line_length * cos(angle),
               center_y + line_length * sin(angle),
               2, color);
    }
  }

  svg_close(svg);
}

// Calculate local ridge frequency in a specific region
float calculate_local_ridge_frequency(Image* im, int x, int y, float angle, int window_size) {
  // Ensure window size is odd
  if (window_size % 2 == 0) window_size++;
  
  // Half window size
  int half_window = window_size / 2;
  
  // Make sure the window is within image bounds
  if (x < half_window || y < half_window || 
      x + half_window >= im->width || 
      y + half_window >= im->height) {
    return 0.0; // Return 0 for invalid regions
  }
  
  // Create a projection along the direction perpendicular to ridge orientation
  float* projection = malloc(sizeof(float) * window_size);
  for (int i = 0; i < window_size; i++) {
    projection[i] = 0.0;
  }
  
  // Calculate the direction perpendicular to ridge orientation
  float cos_angle = cos(angle + PI/2);
  float sin_angle = sin(angle + PI/2);
  
  // Project the image along this direction
  for (int i = -half_window; i <= half_window; i++) {
    for (int j = -half_window; j <= half_window; j++) {
      // Calculate the position in the projection
      int proj_idx = (int)(i * cos_angle + j * sin_angle) + half_window;
      
      // Ensure the projection index is valid
      if (proj_idx >= 0 && proj_idx < window_size) {
        // Get the pixel value at (x+j, y+i)
        int px = x + j;
        int py = y + i;
        
        if (px >= 0 && px < im->width && py >= 0 && py < im->height) {
          projection[proj_idx] += im->p[py][px].r;
        }
      }
    }
  }
  
  // Normalize the projection
  float min_val = 255.0;
  float max_val = 0.0;
  for (int i = 0; i < window_size; i++) {
    if (projection[i] < min_val) min_val = projection[i];
    if (projection[i] > max_val) max_val = projection[i];
  }
  
  if (max_val - min_val < EPSILON) {
    free(projection);
    return 0.0; // No variation in projection
  }
  
  for (int i = 0; i < window_size; i++) {
    projection[i] = (projection[i] - min_val) / (max_val - min_val);
  }
  
  // Find peaks in the projection
  int* peaks = malloc(sizeof(int) * window_size);
  int peak_count = 0;
  
  for (int i = 1; i < window_size - 1; i++) {
    if (projection[i] > projection[i-1] && projection[i] > projection[i+1] && projection[i] > 0.7) {
      peaks[peak_count++] = i;
    }
  }
  
  // Calculate average distance between peaks
  float avg_distance = 0.0;
  if (peak_count >= 2) {
    int total_distances = 0;
    for (int i = 1; i < peak_count; i++) {
      avg_distance += peaks[i] - peaks[i-1];
      total_distances++;
    }
    
    if (total_distances > 0) {
      avg_distance /= total_distances;
    } else {
      avg_distance = 0.0;
    }
  }
  
  // Clean up
  free(projection);
  free(peaks);
  
  // Convert distance to frequency (cycles per pixel)
  if (avg_distance > 2.0) { // Minimum reasonable ridge width
    return 1.0 / avg_distance;
  } else {
    return 0.0; // Invalid frequency
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <input_image> [output_prefix]\n", argv[0]);
    return 1;
  }
  
  // Default output prefix
  char* output_prefix = "fingerprint";
  if (argc >= 3) {
    output_prefix = argv[2];
  }
  
  // Open the input image
  Image* im = ppm_open(argv[1]);
  if (!im) {
    printf("Error: Could not open image %s\n", argv[1]);
    return 1;
  }
  
  int block_size = 3;

  // Compute fingerprint orientation field
  Fingerprint* fp = compute_fingerprint(im, block_size);
  
  // Create output filenames
  char svg_filename[256];
  snprintf(svg_filename, sizeof(svg_filename), "%s.svg", output_prefix);

  // Draw orientation field as SVG
  draw_svg(fp, svg_filename);
  printf("Saved orientation field to %s\n", svg_filename);
  
  // Clean up
  free_fingerprint(fp);
  ppm_free(im);
  
  printf("Processing complete.\n");
  return 0;
}
