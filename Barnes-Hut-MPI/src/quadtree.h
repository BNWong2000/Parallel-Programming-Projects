#ifndef QUADTREE_H
#define QUADTREE_H

#include <utility>
#include <tuple>

#include "body.h"
#include "quadrant.h"
#include "helpers.h"

class QuadTree {
public:
    Body *body;
    QuadTree *topLeft = nullptr;
    QuadTree *topRight = nullptr;
    QuadTree *botLeft = nullptr;
    QuadTree *botRight = nullptr;
    Quadrant *quadrant;

    int bodyCount = 0;
    void createChildren();
    void insertBodyIntoChild(Body *newBody);
    void updateEffectiveBody(Body *newBody);
    QuadTree(Quadrant *quadrant) : quadrant(quadrant) { };
    QuadTree(double xDim, double yDim);
    ~QuadTree();
    
    QuadTree *getTopLeft();
    QuadTree *getTopRight();
    QuadTree *getBotLeft();
    QuadTree *getBotRight();
    Quadrant *getQuadrant();
    int getBodyCount();
    Body *getBody();

    // This might be deleted...
    void setBody(Body *body);

    void insert(Body *newBody);

    void print(int tabLevel);

    std::pair<double, double> calcForceOn(Body *theBody, double theta);
};

#endif