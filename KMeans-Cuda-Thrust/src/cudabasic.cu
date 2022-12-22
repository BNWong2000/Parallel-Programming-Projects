#include "cudabasic.h"
#include <cuda.h>
#include <stdio.h>

// #define TIME_MEMCPY

#define flatIndex(i, j, n) (j + (i * n))


/**
 * Executes a thread per point. 
 */
__global__ void cudaBasicFindNearestCentroids(  int *labels, 
                                                float *centroids, 
                                                float *points, 
                                                int nVals, 
                                                int nCentroids, 
                                                int dimension, 
                                                float minDist   ){
    // for each point compare with each centroid 

    // find index from thread/block id
    int index = threadIdx.x + (blockIdx.x * blockDim.x);
    if(index < nVals){
        float closestDist = minDist;

        // iterate through centroids and compare.
        for(int i = 0; i < nCentroids; i++){

            // calculate euclidean distance. 
            float dist = 0.0f;
            for(int j = 0; j < dimension; j++){
                float temp = points[flatIndex(index, j, dimension)] - centroids[flatIndex(i, j, dimension)];
                temp *= temp;
                dist += temp;
            }
            dist = sqrtf(dist);

            if(dist < closestDist){
                closestDist = dist;
                labels[index] = i;
            }
        }
    }
}

__global__ void cudaBasicResetCentroidsToZero( float *centroids, int *nPointsPerCentroid, int nElements){
    int index = threadIdx.x + (blockIdx.x * blockDim.x);
    if(index < nElements){
        centroids[index] = 0.0f;
        nPointsPerCentroid[index] = 0;
    }
}

__global__ void cudaBasicSumPointsToCentroids(  int *labels,
                                                int *nPointsPerCentroid,
                                                float *centroids,
                                                float *points,
                                                int nCentroids,
                                                int nVals,
                                                int dimension ){
    
    // index is the point number. (1 thread per point)
    int index = threadIdx.x + (blockIdx.x * blockDim.x);
    if(index < nVals){
        int labelIndex = labels[index];
        // Increment points per centroid
        atomicAdd(&nPointsPerCentroid[labelIndex], 1);
        // add the point to the centroid
        for(int i = 0; i < dimension; i++){
            atomicAdd(&centroids[flatIndex(labelIndex, i, dimension)], points[flatIndex(index, i, dimension)]);
        }
    }
}

__global__ void cudaBasicDivideCentroidsByCount(    int *nPointsPerCentroid,
                                                    float *centroids,
                                                    int nElements,
                                                    int dimension ){

    // index is every float in the centroids array (dimension * number of centroids).                                                 
    int index = threadIdx.x + (blockIdx.x * blockDim.x);
    int centroidIndex = index / dimension;
    if(index < nElements && nPointsPerCentroid[centroidIndex] > 0){
        centroids[index] /= nPointsPerCentroid[centroidIndex];
    }
}

__global__ void cudaBasicConverged( float *centroids,
                                    float *oldCentroids,
                                    int nCentroids,
                                    int dimension,
                                    double threshold,
                                    int *notConvergedFlag ){

    // index is the centroid number. 
    int index = threadIdx.x + (blockIdx.x * blockDim.x);
    if(index < nCentroids){
        float dist = 0.0f;
        for(int i = 0; i < dimension; i++){
            float temp = centroids[flatIndex(index, i, dimension)] - oldCentroids[flatIndex(index, i, dimension)];
            temp *= temp;
            dist += temp;
        }
        dist = sqrtf(dist);

        if(dist > threshold){
            atomicAdd(notConvergedFlag, 1);
        }
    }
}

void kmeans_cuda_basic(args_t *args){
    // TIMING:

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
#ifdef TIME_MEMCPY
    cudaEvent_t e2eStart, e2eStop;
    cudaEventCreate(&e2eStart);
    cudaEventCreate(&e2eStop);
    cudaEventRecord(e2eStart);

#endif

    float *d_centroids;
    float *d_oldCentroids;
    float *d_points;
    int *labels;
    int *nPointsPerCentroid;
    int notConvergedFlag = 0;
    int *d_notConvergedFlag;
    cudaMalloc((void **)&d_centroids, sizeof(float) * args->num_cluster * args->dimension);
    cudaMalloc((void **)&d_oldCentroids, sizeof(float) * args->num_cluster * args->dimension);
    cudaMalloc((void **)&d_points, sizeof(float) * args->nVals * args->dimension);
    cudaMalloc((void **)&labels, sizeof(int) * args->nVals); // an array that maps to the index of the nearest centroid
    cudaMalloc((void **)&nPointsPerCentroid, sizeof(int) * args->num_cluster);
    cudaMalloc((void **)&d_notConvergedFlag, sizeof(int));

#ifdef TIME_MEMCPY
    float temp;
    float totalMemCpyTime = 0.0f;
    cudaEvent_t memoryCopyStart, memoryCopyStop;
    cudaEventCreate(&memoryCopyStart);
    cudaEventCreate(&memoryCopyStop);
    cudaEventRecord(memoryCopyStart);
#endif
    cudaMemcpy(d_notConvergedFlag, &notConvergedFlag, sizeof(int), cudaMemcpyHostToDevice);

#ifdef TIME_MEMCPY
    cudaEventRecord(memoryCopyStop);
    cudaEventSynchronize(memoryCopyStop);
    cudaEventElapsedTime(&temp, memoryCopyStart, memoryCopyStop);
    totalMemCpyTime += temp;
#endif

    float *tempCentroids = (float *)malloc(sizeof(float) * args->num_cluster * args->dimension);
    float *tempPoints = (float *)malloc(sizeof(float) * args->nVals * args->dimension);

    // generate k random centroids,
    kmeans_srand(args->seed); // cmd_seed is a cmdline arg
    for (int i=0; i < args->num_cluster; i++){
        int index = kmeans_rand() % args->nVals;
        memcpy(&tempCentroids[i * args->dimension], args->input_vals[index], args->dimension * sizeof(float));
    }

    for (int i = 0; i < args->nVals; i++){
        int index = i * args->dimension;
        memcpy(&tempPoints[index], args->input_vals[i], args->dimension * sizeof(float));
    }
#ifdef TIME_MEMCPY
    cudaEventRecord(memoryCopyStart);
#endif
    cudaMemcpy(d_centroids, tempCentroids, sizeof(float) * args->num_cluster * args->dimension, cudaMemcpyHostToDevice);
    cudaMemcpy(d_points, tempPoints, sizeof(float) * args->nVals * args->dimension, cudaMemcpyHostToDevice);
#ifdef TIME_MEMCPY
    cudaEventRecord(memoryCopyStop);
    cudaEventSynchronize(memoryCopyStop);
    cudaEventElapsedTime(&temp, memoryCopyStart, memoryCopyStop);
    totalMemCpyTime += temp;
#endif
    free(tempCentroids);
    free(tempPoints);
    int iters = 0;
    bool done = false;

    int nThreadsPerBlock = 1024;

    // for kernels that need to run for every point. 
    // taking the integer ceiling.
    int nBlocksA = (args->nVals / nThreadsPerBlock) + ((args->nVals % nThreadsPerBlock) != 0);

    // for kernels that need to run for every dimension of every centroid
    int nElements = args->num_cluster * args->dimension;
    int nBlocksB = (nElements / nThreadsPerBlock) + ((nElements % nThreadsPerBlock) != 0);

    // for kernels that need to run for every centroid. 
    int nBlocksC = (args->num_cluster / nThreadsPerBlock) + ((args->num_cluster % nThreadsPerBlock) != 0);
    float totalKernalTime = 0.0f;

    cudaEventRecord(start);

    while(!done){
        // Reset the notconverged flag
#ifdef TIME_MEMCPY
        cudaEventRecord(memoryCopyStart);
#endif

        cudaMemcpy(d_notConvergedFlag, &notConvergedFlag, sizeof(int), cudaMemcpyHostToDevice);
        cudaMemcpy(d_oldCentroids, d_centroids, sizeof(float) * args->num_cluster * args->dimension, cudaMemcpyDeviceToDevice);

#ifdef TIME_MEMCPY
        cudaEventRecord(memoryCopyStop);
        cudaEventSynchronize(memoryCopyStop);
        cudaEventElapsedTime(&temp, memoryCopyStart, memoryCopyStop);
        totalMemCpyTime += temp;
#endif
        iters++;
        
        // find nearest centroid
        
        cudaBasicFindNearestCentroids<<<nBlocksA,nThreadsPerBlock>>>(
                labels, 
                d_centroids, 
                d_points, 
                args->nVals, 
                args->num_cluster, 
                args->dimension, 
                std::numeric_limits<float>::max() );

        
        
        // Reset Centroids to zero, to take average. 
        cudaBasicResetCentroidsToZero<<<nBlocksB, nThreadsPerBlock>>>(d_centroids, nPointsPerCentroid, args->num_cluster * args->dimension);
        
        cudaBasicSumPointsToCentroids<<<nBlocksA, nThreadsPerBlock>>>(
                labels,
                nPointsPerCentroid,
                d_centroids,
                d_points,
                args->num_cluster,
                args->nVals,
                args->dimension );

        
        cudaBasicDivideCentroidsByCount<<<nBlocksB, nThreadsPerBlock>>>(
                nPointsPerCentroid,
                d_centroids,
                args->num_cluster * args->dimension,
                args->dimension );

        cudaBasicConverged<<<nBlocksC, nThreadsPerBlock>>>( d_centroids,
                                                            d_oldCentroids,
                                                            args->num_cluster,
                                                            args->dimension,
                                                            args->threshold,
                                                            d_notConvergedFlag );
#ifdef TIME_MEMCPY
        cudaEventRecord(memoryCopyStart);
#endif

        cudaMemcpy(&notConvergedFlag, d_notConvergedFlag, sizeof(int), cudaMemcpyDeviceToHost);
#ifdef TIME_MEMCPY
        cudaEventRecord(memoryCopyStop);
        cudaEventSynchronize(memoryCopyStop);
        cudaEventElapsedTime(&temp, memoryCopyStart, memoryCopyStop);
        totalMemCpyTime += temp;
#endif

        done = iters > args->max_num_iter || (notConvergedFlag == 0); //args->max_num_iter
        notConvergedFlag = 0;
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&totalKernalTime, start, stop);

    args->iters = iters;
    args->timeTaken = totalKernalTime/iters;

    args->centroids = (float **)malloc(sizeof(float *) * args->num_cluster);
    for(int i = 0; i < args->num_cluster; i++){
        args->centroids[i] = (float *) malloc(sizeof(float) * args->dimension);
        cudaMemcpy(args->centroids[i], &d_centroids[args->dimension * i], sizeof(float) * args->dimension, cudaMemcpyDeviceToHost);
    }
    args->labels = (int *)malloc(sizeof(int) * args->nVals);
    cudaMemcpy(args->labels, labels, sizeof(int) * args->nVals, cudaMemcpyDeviceToHost);

    
    cudaFree(d_centroids);
    cudaFree(d_oldCentroids);
    cudaFree(d_points);
    cudaFree(labels);
    cudaFree(nPointsPerCentroid);
    cudaFree(d_notConvergedFlag);
    // free everything here...


    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    
#ifdef TIME_MEMCPY
    cudaEventRecord(e2eStop);
    cudaEventSynchronize(e2eStop);
    float totalTime;
    cudaEventElapsedTime(&totalTime, e2eStart, e2eStop);
    printf("memcpy time: %lf, fractional time: %lf\n",totalMemCpyTime, (totalMemCpyTime)/totalTime);
    totalMemCpyTime /= iters;
#endif
}