/*
* Uses forks to create a set of images that can be used to create a movie of the Mandelbrot set.
* Each image is a zoomed in version of the previous image.
* The images are stored in the current directory as mandel_0000.jpg, mandel_0001.jpg, etc.
* The number of images to create is given as a command line argument.
* The maximum number of iterations per point is also given as a command line argument.
* The default values are 1000 images and 1000 iterations per point.
* The default output file is mandel.jpg.
* The default center point is (0,0) and the default scale is 4.
* The default image size is 1000x1000 pixels.   
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "jpegrw.h"  
#include "mandel.h"
#include <sys/wait.h> 


int main(int argc, char *argv[])
{
    int num_images = 1000; // default number of images
    int max_iterations = 1000; // default max iterations
    double xcenter = 0.0; // default center x
    double ycenter = 0.0; // default center y
    double scale = 4.0; // default scale
    int image_width = 1000; // default image width
    int image_height = 1000; // default image height

    // Parse command line arguments for number of images and max iterations
    int opt;
    while ((opt = getopt(argc, argv, "n:m:x:y:s:W:H:")) != -1) {
        switch (opt) {
            case 'n':
                num_images = atoi(optarg);
                break;
            case 'm':
                max_iterations = atoi(optarg);
                break;
            case 'x':
                xcenter = atof(optarg);
                break;
            case 'y':
                ycenter = atof(optarg);
                break;
            case 's':
                scale = atof(optarg);
                break;
            case 'W':
                image_width = atoi(optarg);
                break;
            case 'H':
                image_height = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-n num_images] [-m max_iterations] [-x xcenter] [-y ycenter] [-s scale] [-W width] [-H height]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_images; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Child process
            char filename[256];
            snprintf(filename, sizeof(filename), "mandel_%04d.jpg", i);
            imgRawImage* img = initRawImage(image_width, image_height);
            //Zoom in by reducing the scale for each image
            double current_scale = scale / pow(1.10, i);// Halve the scale for each
            compute_image(img, xcenter - current_scale / 2, xcenter + current_scale / 2, ycenter - current_scale / 2, ycenter + current_scale / 2, max_iterations);
            storeJpegImageFile(img, filename);
            freeRawImage(img);
            exit(0); // Child process exits after creating the image
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // Parent process continues to next iteration to fork the next child
    }  
    for(int i = 0; i < num_images; i++){
        wait(NULL);
    }
    return 0;
} 