#ifndef SEQUENTIAL_H
#define SEQUENTIAL_H

#include "helpers.h"
#include <iostream>
#include <limits>
#include <math.h>

void kmeans_sequential(args_t *args);

void copyCentroids(float **src, float **dest, int nVals, int dim);

void findNearestCentroids(int * labels, args_t *args);

void averageLabeledCentroids(args_t *args, int *labels);

bool converged(args_t *args, float **newCentroids);

#endif