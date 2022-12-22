#include "thrust.h"
#include <stdio.h>
#include <chrono>


#define flatIndex(i, j, n) (j + (i * n))

struct squareRoot : public thrust::unary_function<float, float>{
    __host__ __device__
    float operator()(float input){
        return sqrtf(input);
    }
};

struct subAndSquare : public thrust::binary_function<float, float, float>{
    __host__ __device__
    float operator()(float x, float y){
        return (x-y) * (x-y);
    }
};

struct modByCount : public thrust::unary_function<int, int>{
    int num;

public:
    __host__ __device__ 
    modByCount(int x) : num(x) {}

    __host__ __device__
    int operator()(int input){
        return input % num;
    }
};

struct divByCount : public thrust::unary_function<int, int>{
    int num;

public:
    __host__ __device__ 
    divByCount(int x) : num(x) {}

    __host__ __device__
    int operator()(int input){
        return input / num;
    }
};

struct getPointInd : public thrust::unary_function<int, int>{
    int num;
    int dim; 

public:
    __host__ __device__ 
    getPointInd(int x, int y) : num(x), dim(y) {}

    __host__ __device__
    int operator()(int input){
        int pointIndex = input / num;
        int temp = (input % num) % dim; 
        return (pointIndex * dim) + temp;
    }
};

struct makePtrOffset : public thrust::unary_function<int, float>{
    floatIter initialPtr;

public:
    __host__ __device__ 
    makePtrOffset(floatIter x) : initialPtr(x) {}

    __host__ __device__
    float operator()(int input){
        return (*(initialPtr + input));
    }
};

typedef thrust::tuple<int, float> labelPair;
struct calcMin : public thrust::binary_function<labelPair, labelPair, labelPair>{
    __host__ __device__
    labelPair operator()(labelPair x, labelPair y){
        if(thrust::get<1>(x) < thrust::get<1>(y)){
            return x;
        }else{
            return y;
        }
    }
};

struct makeSumIndices : public thrust::unary_function<int, int>{
    intIter labelVec;
    int dimension;
public:
    __host__ __device__ 
    makeSumIndices(intIter x, int y) : labelVec(x), dimension(y) {}

    __host__ __device__
    int operator()(int input){
        int temp = input % dimension;
        int temp2 = input / dimension;
        return ((*(labelVec + temp2)) * dimension) + temp; 
    }
};

struct getAtIndex : public thrust::unary_function<int, float>{
    floatIter points;
public:
    __host__ __device__ 
    getAtIndex(floatIter x) : points(x) {}

    __host__ __device__
    float operator()(int input){
        return (*(points + input));
    }
};

struct doubleAdd : public thrust::binary_function<labelPair, labelPair, labelPair>{
    __host__ __device__
    labelPair operator()(labelPair x, labelPair y){
        return thrust::make_tuple(thrust::get<0>(x) + thrust::get<0>(y), thrust::get<1>(x) + thrust::get<1>(y));
    }
};

struct divFloatByInt : public thrust::binary_function<float, int, float>{
    __host__ __device__
    float operator()(float x, int y){
        return x / y;
    }
};

struct compareThreshold : public thrust::binary_function<float, double, bool>{
    __host__ __device__
    float operator()(float x, double y){
        return x < y;
    }
};

struct boolAnd : public thrust::binary_function<bool, bool, bool>{
    __host__ __device__
    bool operator()(bool x, bool y){
        return x && y;
    }
};

bool thrustConverged(   floatVec &oldCentroids,
                        floatVec &centroids,
                        int nCentroids,
                        int dimension,
                        double threshold ){
    // 
    floatVec diffSquares(nCentroids * dimension);

    thrust::transform(thrust::device, centroids.begin(), centroids.end(), oldCentroids.begin(), diffSquares.begin(), subAndSquare());
    intVec indices(nCentroids * dimension);
    thrust::sequence(thrust::device, indices.begin(), indices.end(), 0);
    thrust::transform(thrust::device, indices.begin(), indices.end(), indices.begin(), divByCount(dimension));

    intVec garbage(nCentroids * dimension);
    floatVec distances(nCentroids);

    thrust::reduce_by_key(thrust::device, indices.begin(), indices.end(), diffSquares.begin(), garbage.begin(), distances.begin());
    thrust::transform(thrust::device, distances.begin(), distances.end(), distances.begin(), squareRoot());
    thrust::device_vector<bool> isConverged(nCentroids);
    thrust::device_vector<double> thresholdVec(nCentroids);
    thrust::fill(thrust::device, thresholdVec.begin(), thresholdVec.end(), threshold);
    thrust::transform(thrust::device, distances.begin(), distances.end(), thresholdVec.begin(), isConverged.begin(), compareThreshold());
    
    bool result = thrust::reduce(thrust::device, isConverged.begin(), isConverged.end(), true, boolAnd());
    return result;
}

void thrustTakeAverage( floatVec &points, 
                        floatVec &centroids,
                        intVec &labels,
                        int nPoints,
                        int nCentroids,
                        int dimension ){
    intVec labelIndices(nPoints * dimension);

    intVec counts(nPoints * dimension);
    floatVec sums(nPoints * dimension);

    thrust::sequence(thrust::device, labelIndices.begin(), labelIndices.end(), 0);
    thrust::transform(thrust::device, labelIndices.begin(), labelIndices.end(), labelIndices.begin(), makeSumIndices(labels.begin(), dimension));

    thrust::copy(thrust::device, points.begin(), points.end(), sums.begin());

    thrust::stable_sort_by_key(thrust::device, labelIndices.begin(), labelIndices.end(), sums.begin(), thrust::less<int>());

    intVec outputLabels(nCentroids * dimension);

    floatVec outputFloats(nCentroids * dimension);
    intVec outputCounts(nCentroids * dimension);

    thrust::fill(thrust::device, counts.begin(), counts.end(), 1);
    thrust::reduce_by_key(thrust::device, labelIndices.begin(), labelIndices.end(),  sums.begin(), outputLabels.begin(), outputFloats.begin());

    thrust::reduce_by_key(thrust::device, labelIndices.begin(), labelIndices.end(), thrust::make_zip_iterator(thrust::make_tuple(counts.begin(), sums.begin())), outputLabels.begin(), thrust::make_zip_iterator(thrust::make_tuple(outputCounts.begin(), outputFloats.begin())), thrust::equal_to<int>(), doubleAdd());
    thrust::transform(thrust::device, outputFloats.begin(), outputFloats.end(), outputCounts.begin(), centroids.begin(), divFloatByInt());
}


void thrustFindNearestCentroid( floatVec &points, 
                                floatVec &centroids,
                                intVec &labels,
                                int nPoints,
                                int nCentroids,
                                int dimension ){
    // spread out points into centroids
    intVec pointIndices(nPoints * nCentroids * dimension);
    intVec centIndices(nPoints * nCentroids * dimension);

    floatVec centInputs(nPoints * nCentroids * dimension);
    floatVec pointInputs(nPoints * nCentroids * dimension);

    thrust::sequence(thrust::device, pointIndices.begin(), pointIndices.end(), 0);
    thrust::sequence(thrust::device, centIndices.begin(), centIndices.end(), 0);

    thrust::transform(thrust::device, pointIndices.begin(), pointIndices.end(), pointIndices.begin(), getPointInd(nCentroids * dimension, dimension));
    thrust::transform(thrust::device, centIndices.begin(), centIndices.end(), centIndices.begin(), modByCount(nCentroids * dimension));    

    thrust::transform(thrust::device, centIndices.begin(), centIndices.end(), centInputs.begin(), makePtrOffset(centroids.begin()));
    thrust::transform(thrust::device, pointIndices.begin(), pointIndices.end(), pointInputs.begin(), makePtrOffset(points.begin()));

    floatVec distancesIntermediate(nPoints * nCentroids * dimension);
    thrust::transform(thrust::device, pointInputs.begin(), pointInputs.end(), centInputs.begin(), distancesIntermediate.begin(), subAndSquare());

    // reset centIndices
    thrust::sequence(thrust::device, centIndices.begin(), centIndices.end(), 0);
    thrust::transform(thrust::device, centIndices.begin(), centIndices.end(), centIndices.begin(), divByCount(dimension)); 


    floatVec distances(nPoints * nCentroids);
    intVec distanceIndices(nPoints * nCentroids); 

    // We use centIndices as a key for a reduce by key
    thrust::reduce_by_key(thrust::device, centIndices.begin(), centIndices.end(), distancesIntermediate.begin(), distanceIndices.begin(), distances.begin() );
    thrust::transform(thrust::device, distances.begin(), distances.end(), distances.begin(), squareRoot());
    thrust::transform(thrust::device, distanceIndices.begin(), distanceIndices.end(), distanceIndices.begin(), modByCount(nCentroids));

    intVec tempKey(nPoints * nCentroids);
    thrust::sequence(thrust::device, tempKey.begin(), tempKey.end());
    thrust::transform(thrust::device, tempKey.begin(), tempKey.end(), tempKey.begin(), divByCount(nCentroids));
    thrust::reduce_by_key(thrust::device, tempKey.begin(), tempKey.end(), thrust::make_zip_iterator(thrust::make_tuple(distanceIndices.begin(),distances.begin())), pointIndices.begin(), thrust::make_zip_iterator(thrust::make_tuple(labels.begin(), distancesIntermediate.begin())), thrust::equal_to<int>(), calcMin());
}

void kmeans_cuda_thrust(args_t *args){
    thrust::device_vector<float> points(args->nVals * args->dimension);
    thrust::device_vector<float> centroids(args->num_cluster * args->dimension);
    thrust::device_vector<float> oldCentroids(args->num_cluster * args->dimension);
    thrust::device_vector<int> labels(args->nVals);

    // generate k random centroids,
    kmeans_srand(args->seed); // cmd_seed is a cmdline arg
    for (int i = 0; i < args->num_cluster; i++){
        int index = kmeans_rand() % args->nVals;
        for(int j = 0; j < args->dimension; j++){
            centroids[flatIndex(i,j,args->dimension)] = args->input_vals[index][j];
        }
    }

    for (int i = 0; i < args->nVals; i++){
        for(int j = 0; j < args->dimension; j++){
            points[flatIndex(i,j,args->dimension)] = args->input_vals[i][j];
        }
    }
    
    int iters = 0;
    bool done = false;
    const auto start = std::chrono::high_resolution_clock::now();
    while(!done){

        // copy centroids into old centroids
        thrust::copy(centroids.begin(), centroids.end(), oldCentroids.begin());
        iters++;

        // find nearest centroid
        thrustFindNearestCentroid(points, centroids, labels, args->nVals, args->num_cluster, args->dimension);

        thrust::fill(thrust::device, centroids.begin(), centroids.end(), 0.0f);

        thrustTakeAverage(points, centroids, labels, args->nVals, args->num_cluster, args->dimension);

        done = iters > args->max_num_iter || thrustConverged(oldCentroids, centroids, args->num_cluster, args->dimension, args->threshold);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = end - start;

    args->iters = iters;
    args->timeTaken = fp_ms.count()/iters;

    args->centroids = (float **)malloc(sizeof(float *) * args->num_cluster);
    for(int i = 0; i < args->num_cluster; i++){
        args->centroids[i] = (float *) malloc(sizeof(float) * args->dimension);
        for(int j = 0; j < args->dimension; j++){
            args->centroids[i][j] = centroids[flatIndex(i,j,args->dimension)];
        }
    }
    args->labels = (int *)malloc(sizeof(int) * args->nVals);
    for(int i = 0; i < args->nVals; i++){
        args->labels[i] = labels[i];
    }
}