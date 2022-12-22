#ifndef QUADRANT_H
#define QUADRANT_H

#include "math.h"
#include <iostream>

class Quadrant {
public:
    double xMin;
    double yMin;
    double xMax;
    double yMax; 

    Quadrant();
    ~Quadrant() = default;
    Quadrant(double xMinimum, double yMinimum, double xMaximum, double yMaximum)
            : xMin(xMinimum), yMin(yMinimum), xMax(xMaximum), yMax(yMaximum) { }

    double getYHalfway();
    double getXHalfway();

    double getXMin();
    double getYMin();
    double getXMax();
    double getYMax();

    Quadrant* newTopLeft();
    Quadrant* newTopRight();
    Quadrant* newBotLeft();
    Quadrant* newBotRight();
    void print();
};

#endif