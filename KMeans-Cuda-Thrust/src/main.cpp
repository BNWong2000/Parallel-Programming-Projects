#include "argparse.h"
#include "helpers.h"
#include "io.h"
#include "sequential.h"
#include "cudabasic.h"
#include "cudashmem.h"
#include "thrust.h"


int main(int argc, char* argv[]){
    options_t opts;
    args_t arguments;

    get_opts(argc, argv, &opts);
    copyArgs(&arguments, &opts);

    readFile(opts.inputFileName, &arguments);
    switch (opts.mode){
        case 1:
            // sequential
            kmeans_sequential(&arguments);
            break;
        case 2:
            // cuda basic
            kmeans_cuda_basic(&arguments);
            break;
        case 3:
            // cuda shmem
            kmeans_cuda_shmem(&arguments);
            break;
        case 4:
            // Thrust
            kmeans_cuda_thrust(&arguments);
            break;
        default:
            exit(1);
    };

    printf("%d,%lf\n", arguments.iters, arguments.timeTaken);
    if(arguments.outputCentroids){
        for(int i = 0; i < arguments.num_cluster; i++){
            printf("%d ", i);
            for(int j = 0; j < arguments.dimension; j++){
                printf("%lf ", arguments.centroids[i][j]);
            }
            printf("\n");
        }
    }else{
        printf("clusters:");
        for(int i = 0; i < arguments.nVals; i++){
            printf(" %d", arguments.labels[i]);
        }
    }

}