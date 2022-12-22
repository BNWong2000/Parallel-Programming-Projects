#pragma once

#include <stdlib.h>
#include <pthread.h>
#include <barrier.h>
#include <iostream>

void* compute_prefix_sum(void* a);
