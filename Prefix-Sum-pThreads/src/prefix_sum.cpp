#include "prefix_sum.h"
#include "helpers.h"
#include "barrier.h"


void* compute_prefix_sum(void *a)
{
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    // Up Sweep phase
    // jump amount increases by 2x every iteration
    int tid = args->t_id; 
    int nThreads = args->n_threads;

    int jumpAmt = 1;
    Barrier *myBar = args->myBarrier;

    // This is where we upsweep (per Blelloch's algorithm)
    // the amount we jump between elements we want to op() on. 
    for(int i = args->n_vals/2; i > 0; i /= 2){ // i = number of ops to complete per iteration.

        if(tid < i){
            int incrementAmt = i / nThreads;
            int remainder = i % nThreads; 

            // Calculate number of threads and divide up work. 
            // start/end are the operation order to do. 
            int start = incrementAmt * tid; 
            int end = incrementAmt * (tid+1);

            if(tid >= remainder){
                start += remainder;
                end += remainder;
            }else{ // We give extra work to lower number threads first.
                start += tid;
                end += tid+1; // extra work that isn't done by every thread. 
            }

            for(int j = start; j < end; j++){
                // Add numbers and put in output array. 
                int left = jumpAmt*(2*j) + jumpAmt - 1; // left index for op
                int right = jumpAmt*((2*j)+1) + jumpAmt - 1; // right index for op
                args->output_vals[right] = op(args->output_vals[left], args->output_vals[right], args->n_loops);
            }
        }

        jumpAmt *= 2;
        myBar->wait();
        //pthread_barrier_wait(&myBarrier); 
    }

    // Down Sweep Phase
    myBar->wait();
    int lastValue = 0;
    if(tid == 0){
        lastValue = args->output_vals[args->n_vals-1];
        args->output_vals[args->n_vals-1] = 0;
        
    }
    myBar->wait();

    for(int i = 1; i < args->n_vals; i *= 2){
        myBar->wait();
        jumpAmt /= 2;
        
        if(tid < i){
            int incrementAmt = i / nThreads;
            int remainder = i % nThreads; 

            // Calculate number of threads and divide up work.
            // start/end are the operation order to do. 
            int start = incrementAmt * tid; 
            int end = incrementAmt * (tid+1);

            if(tid >= remainder){
                start += remainder;
                end += remainder;
            }else{ // We give extra work to lower number threads first.
                start += tid;
                end += tid+1; // extra work that isn't done by every thread. 
            }

            for(int j = start; j < end; j++){
                // Add numbers and put in output array. 
                int left = jumpAmt*(2*j) + jumpAmt - 1; // left index for op
                int right = jumpAmt*((2*j)+1) + jumpAmt - 1; // right index for op
                int temp = args->output_vals[right];
                args->output_vals[right] = op(args->output_vals[left], args->output_vals[right], args->n_loops);
                args->output_vals[left] = temp;
            }
        }

    }
    myBar->wait();
    if(tid == 0){
        for(int i = 0; i < args->n_vals-1; i++){
            args->output_vals[i] = args->output_vals[i+1];
        }
        args->output_vals[args->n_vals-1] = lastValue;
    }
    myBar->wait();
    return 0;
}
