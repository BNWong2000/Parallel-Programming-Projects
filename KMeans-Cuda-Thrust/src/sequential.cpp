#include "sequential.h"
#include <stdio.h>
#include <string.h>
#include <chrono>

// #define TIME_PARALLELIZABLE_PORTION

void kmeans_sequential(args_t *args){
    // generate k random centroids,
    args->centroids = (float **) malloc(args->num_cluster * sizeof(float *));
    kmeans_srand(args->seed); // cmd_seed is a cmdline arg
    for (int i=0; i < args->num_cluster; i++){
        args->centroids[i] = (float *) malloc(args->dimension * sizeof(float));
        int index = kmeans_rand() % args->nVals;
        // you should use the proper implementation of the following
        // code according to your data structure
        memcpy(args->centroids[i], args->input_vals[index],args->dimension * sizeof(float));
    }
    int iters = 0;
    bool done = false;
    
    int *labels = (int *) malloc(args->nVals * sizeof(int)); // an array that maps to the index of the nearest centroid
    float **oldCentroids = (float **) malloc(args->num_cluster * sizeof(float *));
    for(int i = 0; i < args->num_cluster; i++){
        oldCentroids[i] = (float *) malloc(args->dimension * sizeof(float));
    }

#ifdef TIME_PARALLELIZABLE_PORTION
    double totalParallelizableTime = 0.0f;
#endif
    const auto start = std::chrono::high_resolution_clock::now();
    while(!done){
#ifdef TIME_PARALLELIZABLE_PORTION
        auto parallelizeableStart = std::chrono::high_resolution_clock::now();
#endif
        copyCentroids(args->centroids, oldCentroids, args->num_cluster, args->dimension);
#ifdef TIME_PARALLELIZABLE_PORTION
        auto parallelizeableEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> temp = parallelizeableEnd - parallelizeableStart;
        totalParallelizableTime += temp.count();
#endif
        iters++;

        // find nearest centroid
#ifdef TIME_PARALLELIZABLE_PORTION
        parallelizeableStart = std::chrono::high_resolution_clock::now();
#endif

        findNearestCentroids(labels, args);
        averageLabeledCentroids(args, labels);
        bool convergedYet = converged(args, oldCentroids);

#ifdef TIME_PARALLELIZABLE_PORTION
        parallelizeableEnd = std::chrono::high_resolution_clock::now();
        temp = parallelizeableEnd - parallelizeableStart;
        totalParallelizableTime += temp.count();
#endif
        
        done = iters > args->max_num_iter || convergedYet;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = end - start;
#ifdef TIME_PARALLELIZABLE_PORTION
    printf("parallelizeable time is: %lf, total time is: %lf, fraction is: %lf\n", totalParallelizableTime, fp_ms.count(), totalParallelizableTime / (fp_ms.count()));
#endif

    args->timeTaken = fp_ms.count()/iters;
    args->iters = iters;
    // free everything here...
    args->labels = labels;
}

void copyCentroids(float **src, float **dest, int nVals, int dim){
    for(int i = 0; i < nVals; i++){
        memcpy(dest[i], src[i], dim * sizeof(float));
    }
}

void findNearestCentroids(int *labels, args_t *args){
    // iterate through points
    for(int i = 0; i < args->nVals; i++){
        float closestDist = std::numeric_limits<float>::max();

        // iterate through centroids
        for(int j = 0; j < args->num_cluster; j++){
            // calculate euclidean distance
            float eucDist = 0.0f;
            for(int k = 0; k < args->dimension; k++){
                float temp = args->input_vals[i][k] - args->centroids[j][k];
                temp *= temp;
                eucDist += temp;
            }
            eucDist = sqrtf(eucDist);
            if(eucDist < closestDist){
                // update labels with the index. 
                closestDist = eucDist;
                labels[i] = j;
            }
        }
    }
}

void averageLabeledCentroids(args_t *args, int *labels){

    int *nPointsPerCentroid = (int *) calloc(args->num_cluster, sizeof(int));
    // reset centroids
    for(int i = 0; i < args->num_cluster; i++){
        for(int j = 0; j < args->dimension; j++){
            args->centroids[i][j] = 0.0f;
        }
    }

    for(int i = 0; i < args->nVals; i++){
        int currIndex = labels[i];
        nPointsPerCentroid[currIndex]++;
        for(int j = 0; j < args->dimension; j++){
            args->centroids[currIndex][j] += args->input_vals[i][j];
        }
    }

    for(int i = 0; i < args->num_cluster; i++){
        if(nPointsPerCentroid[i] > 0){
            for(int j = 0; j < args->dimension; j++){
                args->centroids[i][j] /= nPointsPerCentroid[i];
            }   
        }
    }
}

bool converged(args_t *args, float **oldCentroids){
    for(int i = 0; i < args->num_cluster; i++){
        float eucDist = 0.0f;
        for(int j = 0; j < args->dimension; j++){
            float temp = args->centroids[i][j] - oldCentroids[i][j];
            temp *= temp;
            eucDist += temp;
        }
        eucDist = sqrtf(eucDist);
        if(eucDist > args->threshold){
            return false;
        }
    }
    return true;
}