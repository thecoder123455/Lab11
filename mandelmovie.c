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
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "jpegrw.h"  
#include "mandel.h"
#include <sys/wait.h> 

int createThreads(imgRawImage *img, int numThreads, double xcenter,
                  double ycenter, double scale, int max_iterations);

typedef struct thread_data {
    imgRawImage *img;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max_iterations;
    unsigned int first_row;
    unsigned int end_row;
} thread_data_t;

static void *compute_image_thread(void *arg)
{
    thread_data_t *data = arg;
    compute_image_rows(data->img, data->xmin, data->xmax,
                       data->ymin, data->ymax, data->max_iterations,
                       data->first_row, data->end_row);
    return NULL;
}

int main(int argc, char *argv[])
{
    int num_images = 50; // default number of images
    int max_iterations = 1000; // default max iterations
    double xcenter = -0.7435;; // default center x
    double ycenter = 0.1314;; // default center y
    double scale = 0.01; // default scale
    int image_width = 1000; // default image width
    int image_height = 1000; // default image height
    int pthreadNumber = 1; // default thread number
    int numProcesses = 1; // default number of processes

    // Parse command line arguments for number of images and max iterations
    int opt;
    while ((opt = getopt(argc, argv, "i:P:m:x:y:s:W:H:T:")) != -1) {
        switch (opt) {
            case 'i':
                num_images = atoi(optarg);
                break;
            case 'P':
                numProcesses = atoi(optarg);
                break;
            case 'T':
                pthreadNumber = atoi(optarg);
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
                fprintf(stderr, "Usage: %s [-i num_images] [-P numProcesses] [-m max_iterations] [-x xcenter] [-y ycenter] [-s scale] [-W width] [-H height] [-T threadNumber]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (pthreadNumber <= 0) {
        fprintf(stderr, "Thread count must be greater than zero\n");
        return EXIT_FAILURE;
    }
    if(numProcesses <= 0) {
        fprintf(stderr, "Process count must be greater than zero\n");
        return EXIT_FAILURE;
    }
    int activeProcesses = 0;
    for (int i = 0; i < num_images; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Child process
            char filename[256];
            snprintf(filename, sizeof(filename), "mandel_%04d.jpg", i);
            imgRawImage* img = initRawImage(image_width, image_height);
            //Zoom in by reducing the scale for each image
            double current_scale = scale / pow(1.10, i);// Halve the scale for each
            if (createThreads(img, pthreadNumber, xcenter, ycenter, current_scale, max_iterations) != 0) {
                freeRawImage(img);
                exit(EXIT_FAILURE);
            }
            storeJpegImageFile(img, filename);
            freeRawImage(img);
            exit(0); // Child process exits after creating the image
        } else if (pid > 0) {
            activeProcesses++;
          if (activeProcesses >= numProcesses) {
                wait(NULL); // Wait for any child process to finish
                activeProcesses--;
          }
        } else{
             perror("fork");
            exit(EXIT_FAILURE);
          }
        }
        // Parent process continues to next iteration to fork the next child
      
  while(activeProcesses > 0) {
        wait(NULL); // Wait for remaining child processes to finish
        activeProcesses--;
    }

    return 0;
} 

int createThreads(imgRawImage *img, int numThreads, double xcenter, double ycenter, double scale, int max_iterations)
{
    pthread_t *threads = calloc((size_t)numThreads, sizeof(*threads));
    thread_data_t *threadData = calloc((size_t)numThreads, sizeof(*threadData));
    if (threads == NULL || threadData == NULL) {
        perror("calloc");
        free(threads);
        free(threadData);
        return -1;
    }

    double xmin = xcenter - scale / 2.0;
    double xmax = xcenter + scale / 2.0;
    double ymin = ycenter - scale / 2.0;
    double ymax = ycenter + scale / 2.0;
    int created = 0;
    int failed = 0;

    for (int i = 0; i < numThreads; i++) {
        threadData[i].img = img;
        threadData[i].xmin = xmin;
        threadData[i].xmax = xmax;
        threadData[i].ymin = ymin;
        threadData[i].ymax = ymax;
        threadData[i].max_iterations = max_iterations;
        threadData[i].first_row = (unsigned int)((size_t)img->height * (size_t)i / (size_t)numThreads);
        threadData[i].end_row = (unsigned int)((size_t)img->height * (size_t)(i + 1) / (size_t)numThreads);

        int error = pthread_create(&threads[i], NULL, compute_image_thread, &threadData[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            failed = 1;
            break;
        }
        created++;
    }

    for (int i = 0; i < created; i++) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            failed = 1;
        }
    }

    free(threadData);
    free(threads);
    return failed ? -1 : 0;
}