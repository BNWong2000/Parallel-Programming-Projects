#include "helpers.h"

/**
 * 
 * s is cluster size 
 */ 
bool checkMAC (double s, double d, double theta) {
    return (s / d) < theta;
}