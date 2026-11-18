/*
 * mandel.h
 */
#ifndef MANDEL_H
#define MANDEL_H

#include "jpegrw.h"

void compute_image(imgRawImage *img, double xmin, double xmax,
                   double ymin, double ymax, int max);
void compute_image_rows(imgRawImage *img, double xmin, double xmax,
                        double ymin, double ymax, int max,
                        unsigned int first_row, unsigned int end_row);

#endif /* MANDEL_H */
