#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include <string>

struct options_t {
    int num_cluster;
    int dims;
    char *inputFileName;
    int max_num_iter;
    double threshold;
    bool outputCentroids;
    int seed;
    int mode;
};

typedef struct options_t options_t;

void get_opts(int argc,
              char **argv,
              options_t *opts);

#endif