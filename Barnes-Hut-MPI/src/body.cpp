#include "body.h"

void copy(Body &src, Body &dst) {
    dst.index = src.index;
    dst.x = src.x;
    dst.y = src.y;
    dst.m = src.m;
    dst.vx = src.vx;
    dst.vy = src.vy;
    // dst.fx = src.fx;
    // dst.fy = src.fy;
}

void getFromString(Body &body, std::string line) {
    std::stringstream strStream(line);
    std::string temp;
    strStream >> temp;
    body.index = stoi(temp);
    strStream >> temp;
    body.x = stod(temp);
    strStream >> temp;
    body.y = stod(temp);
    strStream >> temp;
    body.m = stod(temp);
    strStream >> temp;
    body.vx = stod(temp);
    strStream >> temp;
    body.vy = stod(temp);
    // body.fx = 0;
    // body.fy = 0;
}

Body *clone(Body *body) {
    Body *res = (Body *)malloc(sizeof(Body));
    res->index = -1;
    res->x = body->x;
    res->y = body->y;
    res->m = body->m;
    res->vx = body->vx;
    res->vy = body->vy;
    // res->fx = 0;
    // res->fy = 0;
    return res;
}

void calcNewPos(Body *body, double dt, double fx, double fy) {
    //std::cout << "body: " << index << ", { " << fx << ", " << fy << " } ";
    double ax = fx/body->m;
    double ay = fy/body->m;
    //std::cout << "pos: " << x << ", " << y;
    body->x = body->x + (body->vx * dt) + (0.5 * ax * dt * dt);
    body->y = body->y + (body->vy * dt) + (0.5 * ay * dt * dt);
    //std::cout << " newPos: " << x << ", " << y << std::endl;
    body->vx = body->vx + (ax * dt);
    body->vy = body->vy + (ay * dt);
}

std::string createLine(Body *body) {
    std::string res = "";
    res += std::to_string(body->index);
    res += "\t";
    res += std::to_string(body->x);
    res += "\t";
    res += std::to_string(body->y);
    res += "\t";
    res += std::to_string(body->m);
    res += "\t";
    res += std::to_string(body->vx);
    res += "\t";
    res += std::to_string(body->vy);
    res += "\n";
    return res;
}

static double G = 0.0001;
static double rLimit = 0.03;

// Direction of force is correct for b1, inverted for b2
std::pair<double, double> calcF(Body *b1, Body *b2) {
    double xDiff = b2->x - b1->x;
    double yDiff = b2->y - b1->y;
    double dist = sqrt((xDiff * xDiff) + (yDiff * yDiff));
    return std::make_pair<double, double> (
            calcDimF(b1->m, b2->m, dist, xDiff), 
            calcDimF(b1->m, b2->m, dist, yDiff));
}

double calcF(double m1, double m2, double d) {
    double temp = d < rLimit ? rLimit : d;
    return ( G * m1 * m2 ) / (temp * temp);
}

double calcDimF(double m1, double m2, double d, double dx) {
    double temp = d < rLimit ? rLimit : d;
    return ( G * m1 * m2 * dx ) / (temp * temp * temp);
}