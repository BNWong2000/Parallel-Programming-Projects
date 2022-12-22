#ifndef _SPIN_BARRIER_H
#define _SPIN_BARRIER_H

#include <pthread.h>
#include <iostream>
#include <atomic>
#include <semaphore.h>

// define CUSTOM_BARRIER

#ifdef CUSTOM_BARRIER

class Barrier {
private:
    int numThreads;
    sem_t arrivalSem;
    sem_t departureSem;
    std::atomic<int> counter;

public:
    Barrier(int nThreads);
    ~Barrier();
    
    void wait();

};

#else

class Barrier {
private:
    pthread_barrier_t myBarrier;
public:
    Barrier(int nThreads);
    ~Barrier();
    void wait();
};

#endif

#endif
