#ifndef IO_H
#define IO_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "helpers.h"
#include "body.h"


void readFile(const char* fileName, options_t *options, std::vector<Body> &bodies);
void write_file(options_t *options, std::vector<Body> &bodies);




#endif