#include "helpers.h"


void copyArgs(args_t *dest, options_t *src){
    dest->dimension = src->dims;
    dest->num_cluster = src->num_cluster;
    dest->max_num_iter = src->max_num_iter;
    dest->threshold = src->threshold;
    dest->outputCentroids = src->outputCentroids;
    dest->seed = src->seed;
}


static unsigned long int next = 1;
static unsigned long kmeans_rmax = 32767;

int kmeans_rand() {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % (kmeans_rmax+1);
}

void kmeans_srand(unsigned int seed) {
    next = seed;
}
