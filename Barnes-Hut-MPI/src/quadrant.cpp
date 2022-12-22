#include "quadrant.h"

Quadrant::Quadrant() {}

double Quadrant::getYHalfway(){
    return yMin + ((yMax - yMin) / 2);
}

double Quadrant::getXHalfway(){
    return xMin + ((xMax - xMin) / 2);
}


double Quadrant::getXMin(){
    return xMin;
}

double Quadrant::getYMin(){
    return yMin;
}

double Quadrant::getXMax(){
    return xMax;
}

double Quadrant::getYMax(){
    return yMax;
}

Quadrant* Quadrant::newTopLeft(){
    return new Quadrant(xMin, getYHalfway(), getXHalfway(), yMax);
}
Quadrant* Quadrant::newTopRight(){
    return new Quadrant(getXHalfway(), getYHalfway(), xMax, yMax);
}
Quadrant* Quadrant::newBotLeft(){
    return new Quadrant(xMin, yMin, getXHalfway(), getYHalfway());
}
Quadrant* Quadrant::newBotRight(){
    return new Quadrant(getXHalfway(), yMin, xMax, getYHalfway());
}

void Quadrant::print() {
    std::cout << " { " << xMin << ", " << yMin << ", " << xMax << ", " << yMax << " }" << std::endl; 
}