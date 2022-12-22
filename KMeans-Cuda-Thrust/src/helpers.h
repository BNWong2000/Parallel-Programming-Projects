#ifndef HELPERS_H
#define HELPERS_H

#include "argparse.h"

struct args_t {
    float **input_vals;
    int dimension;
    int num_cluster;
    int nVals;
    int max_num_iter;
    double threshold;
    bool outputCentroids;
    int seed;
    float **centroids;
    int *labels;
    int iters;
    float timeTaken;
};


typedef struct args_t args_t;

void copyArgs(args_t *dest, options_t *src);

int kmeans_rand();

void kmeans_srand(unsigned int seed);

#endif