#include "gabor.h"
#include "ppm.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.141592
#define EPSILON 1E-6

// Create a Gabor filter kernel
float** create_gabor_kernel(int size, float angle, float frequency, float sigma_x, float sigma_y) {
    float** kernel = malloc(sizeof(float*) * size);
    for (int i = 0; i < size; i++) {
        kernel[i] = malloc(sizeof(float) * size);
    }
    
    int half_size = size / 2;
    float sum = 0.0;
    
    // Precompute sin and cos values
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    
    // Create the Gabor filter
    for (int y = -half_size; y <= half_size; y++) {
        for (int x = -half_size; x <= half_size; x++) {
            // Rotate the coordinates
            float x_theta = x * cos_angle + y * sin_angle;
            float y_theta = -x * sin_angle + y * cos_angle;
            
            // Calculate the Gabor value
            float exp_term = exp(-0.5 * (
                (x_theta * x_theta) / (sigma_x * sigma_x) + 
                (y_theta * y_theta) / (sigma_y * sigma_y)
            ));
            
            float cos_term = cos(2 * PI * frequency * x_theta);
            
            // Store the value in the kernel
            kernel[y + half_size][x + half_size] = exp_term * cos_term;
            sum += kernel[y + half_size][x + half_size];
        }
    }
    
    // Normalize the kernel if sum is not zero
    if (fabs(sum) > EPSILON) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                kernel[i][j] /= sum;
            }
        }
    }
    
    return kernel;
}

// Free the Gabor kernel
void free_gabor_kernel(float** kernel, int size) {
    for (int i = 0; i < size; i++) {
        free(kernel[i]);
    }
    free(kernel);
}
