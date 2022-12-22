#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include <string>

struct options_t {
    char *inputFileName;
    char *outputFileName;
    int steps;
    double theta;
    double timeStep;
    bool visualize;
};

typedef struct options_t options_t;

void get_opts(int argc,
              char **argv,
              options_t *opts);

#endif