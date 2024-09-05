#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include the stb_image header file
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Include the stb_image_write header file
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// returns the minimum value in a vector of N ints
int min_v(int *v, int N){
    int min = INT32_MAX;
    for (int i=0; i<N; i++){
        if (v[i] < min){
            min = v[i];
        }
    }
    return min;
}
// returns the index of the minimum value in a vector of N ints
int min_i(int *v, int N){
    int min = INT32_MAX;
    int min_i = -1;
    for (int i=0; i<N; i++){
        if (v[i] < min){
            min = v[i];
            min_i = i;
        }
    }
    return min_i;
}

/*
Note (1):
at each iteration we remove a seam, so the width of the image is reduced by 1
but we keep the same 2d array, so we need to ignore some pixels to the right
this is why we have the to_ignore_pixls parameter
*/
int *find_min_seam(unsigned char *img, int** brightness, int width, int height, int channels, int to_ignore_pixls){
    // this matrix contains in position (i,j) the minimum path weight from anywhere on top to the (i,j) pixel
    int **M = malloc(sizeof(int*) * height);
    // this matrix contains in position (i,j) the predecessor of that pixel in the min path (-1 left top, 0 top, 1 right top)
    int **P = malloc(sizeof(int*) * height);

    for (int i=0; i<height; i++){
        M[i] = malloc(sizeof(int) * width);
        P[i] = malloc(sizeof(int) * width);
        
        if (i==0){ continue; } // skip first row

        int options[3];
        for (int j=0; j<width; j++){
            options[0] = j==0 ? INT32_MAX : M[i-1][j-1] + abs(brightness[i-1][j-1] - brightness[i][j]);
            options[1] = M[i-1][j] + abs(brightness[i-1][j] - brightness[i][j]);
            options[2] = j==width-1 ? INT32_MAX : M[i-1][j+1] + abs(brightness[i-1][j+1] - brightness[i][j]);

            M[i][j] = min_v(options, 3);
            P[i][j] = min_i(options, 3)-1;
        }
    }

    // backtrack
    int *seam = malloc(sizeof(int) * height);
    seam[height-1] = min_i(M[height-1], width);
    for (int i=height-2; i>=0; i--){
        seam[i] = seam[i+1] + P[i+1][seam[i+1]];
    }

    free(M);
    free(P);

    return seam;
}

/*
Debug function used to write the seam to remove
*/
void write_seam_image(unsigned char *img, int width, int height, int channels, int *min_seam, int n, char *filename, int N){
    unsigned char *seam_img = calloc(width * height * 3, sizeof(unsigned char) );
    for (int i=0; i<height; i++){
        int index = (i * width + min_seam[i]) * 3;
        seam_img[index] = 255;
        seam_img[index+1] = 0;
        seam_img[index+2] = 0;
    }
    
    char output_filename[150];
    snprintf(output_filename, 150, "%.*s_%d___%d-esimo seam.png", (int)(strlen(filename) - 4), filename, N, n);

    if (stbi_write_png(output_filename, width, height, 3, seam_img, width * 3) != 0) {
        printf("Image saved successfully\n");
    } else { printf("Failed to save the image\n"); }
}

int main(int argc, char **argv){
    clock_t begin = clock();
    
    int index; // utility variable
    // image info
    int width, height, channels;

    char filename[100];
    if (argc < 3) { printf("Usage: ./seam_carver <image_path> <number of seams to remove>\n"); return 1; }
    strcpy(filename, argv[1]);
    int N = atoi(argv[2]);

    // Load the image using stbi_load
    unsigned char *img = stbi_load(filename, &width, &height, &channels, 0);
    if (img == NULL) { printf("Error in loading the image\n"); return 1; }
    printf("Image loaded successfully\n");

    // create a 2d array of brightness values
    int **brightness = malloc(sizeof(int*) * height);
    index = 0;
    for (int i=0; i<height; i++){
        brightness[i] = malloc(sizeof(int) * width);
        for (int j=0; j<width; j++){
            brightness[i][j] = img[index] + img[index+1] + img[index+2];
            index += channels;
        }
    }

    // remove N seams
    for (int n=0; n<N; n++){
        // find min seam
        int *min_seam = find_min_seam(img, brightness, width, height, channels, n);
        
        // write_seam_image(img, width, height, channels, min_seam, n, filename, N);

        // remove seam from image and brightness array
        for (int i=0; i<height; i++){
            for(int j=min_seam[i]; j<width-1; j++){
                index = (i * (width + n) + j) * channels; // Note (1)
                img[index] = img[index + channels];
                img[index + 1] = img[index + 1 + channels];
                img[index + 2] = img[index + 2 + channels];

                brightness[i][j] = brightness[i][j+1];
            }
        }
        width--;
    }

    // put image in new matrix of correct size
    unsigned char *new_img = malloc(sizeof(unsigned char) * width * height * 3);
    index = 0;
    for (int i=0; i<height; i++){
        int old_image_index = i * (width + N) * channels; // Note (1)
        for (int j=0; j<width; j++){
            new_img[index] = img[old_image_index];
            new_img[index+1] = img[old_image_index+1];
            new_img[index+2] = img[old_image_index+2];
            index += 3;
            old_image_index += channels;
        }
    }

    // write the smaller image
    char output_filename[150];
    snprintf(output_filename, 150, "%.*s_%d.png", (int)(strlen(filename) - 4), filename, N);
    
    if (stbi_write_png(output_filename, width, height, 3, new_img, width * 3) != 0) {
        printf("Image saved successfully\n");
    } else { printf("Failed to save the image\n"); }

    // Free the memory allocated for the image
    stbi_image_free(img);
    stbi_image_free(new_img);
    for (int i=0; i<height; i++){
        free(brightness[i]);
    }
    free(brightness);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Execution time: %f\n", time_spent);
    
    return 0;
}