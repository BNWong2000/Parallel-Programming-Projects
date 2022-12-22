#include "quadtree.h"

QuadTree::QuadTree(double xDim, double yDim){
    quadrant = new Quadrant(0.0, 0.0, xDim, yDim);
    bodyCount = 0;
}

QuadTree::~QuadTree(){
    if(topRight != nullptr){
        // topRight->~QuadTree();
        delete topRight;
    }
    if(botRight != nullptr) {
        // botRight->~QuadTree();
        delete botRight;
    }
    if(topLeft != nullptr){
        // topLeft->~QuadTree();
        delete topLeft;
    }
    if(botLeft != nullptr){
        // botLeft->~QuadTree();
        delete botLeft;
    }
    delete quadrant;
    if(bodyCount > 1) {
        free(body);
    }
}

QuadTree *QuadTree::getTopLeft(){
    return topLeft;
}

QuadTree *QuadTree::getTopRight(){
    return topRight;
}

QuadTree *QuadTree::getBotLeft(){
    return botLeft;
}

QuadTree *QuadTree::getBotRight(){
    return botRight;
}

Quadrant *QuadTree::getQuadrant() {
    return quadrant;
}

Body *QuadTree::getBody(){
    return body;
}

void QuadTree::setBody(Body *body) {
    this->body = body;
}

void QuadTree::insert(Body *newBody){
    if (newBody->m <= 0) {
        // std::cout << "    0 mass" << std::endl;
        newBody->m = -1.0;
        return;
    }
    if(newBody->x > quadrant->getXMax() 
            || newBody->x < quadrant->getXMin() 
            || newBody->y > quadrant->getYMax()
            || newBody->y < quadrant->getYMin() ){

        // std::cout << "    outside of range" << std::endl;
        newBody->m = -1.0;
        return;
    }
    if (bodyCount == 0) {
        this->body = newBody;
        bodyCount++;
        return;
    } else if (bodyCount == 1) {
        // create new sub tree
        // createChildren();
        // std::cout << "    moving " << this->body->getIndex() << " into child Tree" << std::endl;
        insertBodyIntoChild(this->body);
        Body *temp = clone(this->body);
        this->body = temp;
    }
    // std::cout << "    inserting " << newBody->getIndex() <<" as a child" << std::endl;
    insertBodyIntoChild(newBody);
    updateEffectiveBody(newBody);
    bodyCount++;
}

int QuadTree::getBodyCount() {
    return bodyCount;
}

// void QuadTree::createChildren() {
//     topLeft = new QuadTree(quadrant->newTopLeft());
//     topRight = new QuadTree(quadrant->newTopRight());
//     botLeft = new QuadTree(quadrant->newBotLeft());
//     botRight = new QuadTree(quadrant->newBotRight());
// }

void QuadTree::insertBodyIntoChild(Body *newBody) {
    // std::cout << "        { " << newBody->getX() << ", " << newBody->getY() << " } ";
    double xMid = quadrant->getXHalfway(); // botLeft->getQuadrant()->getXMax()
    double yMid = quadrant->getYHalfway(); // botLeft->getQuadrant()->getYMax()
    if (newBody->x < xMid) {
        // left side
        if (newBody->y < yMid) {
            // bottom
            // botLeft->getQuadrant()->print();
            if(botLeft == nullptr) {
                botLeft = new QuadTree(quadrant->newBotLeft());
            }
            botLeft->insert(newBody);
        } else {
            // top
            if(topLeft == nullptr) {
                topLeft = new QuadTree(quadrant->newTopLeft());
            }
            
            topLeft->insert(newBody);
        }
    } else {
        // right side
        if (newBody->y < yMid) {
            // bottom
            if(botRight == nullptr) {
                botRight = new QuadTree(quadrant->newBotRight());
            }
            botRight->insert(newBody);
        } else {
            // top
            if(topRight == nullptr) {
                topRight = new QuadTree(quadrant->newTopRight());
            }
            topRight->insert(newBody);
        }
    }
}

void QuadTree::updateEffectiveBody(Body *newBody) {
    if( newBody->m == 0 ) {
        return;
    }

    double x = body->x;
    double y = body->y;
    double m = body->m;
    x *= m;
    y *= m;
    double newM = newBody->m;
    m += newM;
    x += (newBody->x * newM);
    y += (newBody->y * newM);

    x /= m;
    y /= m;
    body->x =(x);
    body->y =(y);
    body->m =(m);
}

std::pair<double, double> QuadTree::calcForceOn(Body *theBody, double theta) {
    if (bodyCount == 0) {
        // empty node. 
        return {0.0, 0.0};
    } 
    if (bodyCount == 1) {
        if (body->index == theBody->index){
            return {0.0, 0.0};
        } 
        return calcF(theBody, body);
    }
    // internal node...
    double s = quadrant->getXMax() - quadrant->getXMin();
    double xDiff = body->x - theBody->x;
    double yDiff = body->y - theBody->y;
    double d = sqrt((xDiff * xDiff) + (yDiff * yDiff));
    if(checkMAC(s,d, theta)) {
        return calcF(theBody, body);
    } else {
        double resX = 0, resY = 0, tempX, tempY;
        if (botLeft != nullptr) {
            std::tie(tempX, tempY) = botLeft->calcForceOn(theBody, theta);
            resX += tempX;
            resY += tempY;
        }
        if (botRight != nullptr) {
            std::tie(tempX, tempY) = botRight->calcForceOn(theBody, theta);
            resX += tempX;
            resY += tempY;
        }
        if (topLeft != nullptr) {
            std::tie(tempX, tempY) = topLeft->calcForceOn(theBody, theta);
            resX += tempX;
            resY += tempY;
        }
        if (topRight != nullptr) {
            std::tie(tempX, tempY) = topRight->calcForceOn(theBody, theta);
            resX += tempX;
            resY += tempY;
        }
        return {resX, resY};
    }
}


void QuadTree::print(int tabLevel) {
    if(bodyCount == 0) {
        return;
    }
    for(int i = 0; i < tabLevel; i++) {
        std::cout << "----";
    }
    std::cout << "{ index: " << body->index  << ", bodies: " << bodyCount <<" }" << std::endl;
    tabLevel++;
    if(bodyCount > 1) {
        topLeft->print(tabLevel);
        topRight->print(tabLevel);
        botLeft->print(tabLevel);
        botRight->print(tabLevel);
    }

}