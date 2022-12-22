#ifndef THRUST_IMPL_H
#define THRUST_IMPL_H

#include "helpers.h"
#include <iostream>
#include <limits>
#include <math.h>

#include <thrust/device_vector.h>
#include <thrust/copy.h>
#include <thrust/functional.h>
#include <thrust/sequence.h>
#include <thrust/reduce.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/tuple.h>
#include <thrust/fill.h>
#include <thrust/sort.h>

typedef thrust::device_vector<float> floatVec;
typedef thrust::device_vector<float>::iterator floatIter;
typedef thrust::device_vector<floatIter> floatIterVec;
typedef thrust::device_vector<int> intVec;
typedef thrust::device_vector<int>::iterator intIter;

void kmeans_cuda_thrust(args_t *args);

void thrustFindNearestCentroid( floatVec &points, 
                                floatVec &centroids,
                                floatVec &labels,
                                int nPoints,
                                int nCentroids,
                                int dimension);

#endif