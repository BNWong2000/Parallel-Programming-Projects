#ifndef BODY_H
#define BODY_H

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <utility>
#include "math.h"


typedef struct Body {
    int index;
    double x;
    double y;
    double m;
    double vx;
    double vy;
    // double fx;
    // double fy;
} Body;

void copy(Body &src, Body &dst);
void getFromString(Body &body, std::string line);
Body *clone(Body *body);
void calcNewPos(Body *body, double dt, double fx, double fy);
std::string createLine(Body *body);

std::pair<double, double> calcF(Body *b1, Body *b2);

double calcF(double m1, double m2, double d);

double calcDimF(double m1, double m2, double d, double dx);

#endif