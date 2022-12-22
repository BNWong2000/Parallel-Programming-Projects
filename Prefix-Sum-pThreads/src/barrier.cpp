#include <barrier.h>


/************************
 * Your code here...    *
 * or wherever you like *
 ************************/
#ifdef CUSTOM_BARRIER
Barrier::Barrier(int nThreads){
    numThreads = nThreads;
    sem_init(&arrivalSem, 0, 1);
    sem_init(&departureSem, 0, 0);
    counter = 0;
}

Barrier::~Barrier(){
    sem_destroy(&arrivalSem);
    sem_destroy(&arrivalSem);
}

void Barrier::wait(){
    sem_wait(&arrivalSem);
    if(++counter < numThreads){
        sem_post(&arrivalSem);
    }else{
        sem_post(&departureSem);
    }
    sem_wait(&departureSem);
    if(--counter > 0){
        sem_post(&departureSem);
    }else{
        sem_post(&arrivalSem);
    }
}

#else

Barrier::Barrier(int nThreads){
    pthread_barrier_init(&myBarrier, NULL, nThreads);
}

Barrier::~Barrier(){
    pthread_barrier_destroy(&myBarrier);
}

void Barrier::wait(){
    pthread_barrier_wait(&myBarrier);
}

 #endif