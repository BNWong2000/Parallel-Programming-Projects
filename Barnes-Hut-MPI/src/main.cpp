#include <vector>
// -i input/nb-100000.txt -s 10 -t 0.5 -d 0.005 
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glu.h>
#include <GL/glut.h>

#include "argparse.h"
#include "helpers.h"
#include "quadtree.h"
#include "body.h"
#include "io.h"
#include "mpi.h"

#include <unistd.h>


double convert(double coordinate) {
    return 2*coordinate/4.0 - 1;
}

void drawOctreeBounds2D(QuadTree *node) {
    // int i;
    if(node->getBodyCount() <= 1)
        return;
    glBegin(GL_LINES);
    // set the color of lines to be white
    glColor3f(1.0f, 1.0f, 1.0f);
    // specify the start point's coordinates
    double x = node->getQuadrant()->getXMin() + (node->getQuadrant()->getXMax() - node->getQuadrant()->getXMin())/2;
    double y = node->getQuadrant()->getYMin() + (node->getQuadrant()->getYMax() - node->getQuadrant()->getYMin())/2;
    glVertex2f(convert(x), convert(node->getQuadrant()->getYMin()));
    // specify the end point's coordinates
    glVertex2f(convert(x), convert(node->getQuadrant()->getYMax()));
    // do the same for verticle line
    glVertex2f(convert(node->getQuadrant()->getXMin()), convert(y));
    glVertex2f(convert(node->getQuadrant()->getXMax()), convert(y));
    glEnd();
    drawOctreeBounds2D(node->getTopLeft());
    drawOctreeBounds2D(node->getTopRight());
    drawOctreeBounds2D(node->getBotLeft());
    drawOctreeBounds2D(node->getBotRight());
}


void
drawParticle2D(double x_window, double y_window,
           double radius) {
    int k = 0;
    float angle = 0.0f;
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f, 0.0, 0.0);
    for(k=0;k<20;k++){
        angle=(float) (k)/19*2*3.141592;
        glVertex2f(convert(x_window)+radius*cos(angle),
           convert(y_window)+radius*sin(angle));
    }
    glEnd();
}

void run(QuadTree *tree, std::vector<Body> &bodies, double theta, double dt){
    std::vector<std::pair<double, double>> forces(bodies.size());
    for(unsigned int i = 0; i < bodies.size(); i++) {
        if (bodies[i].m > 0) {
            double xForce;
            double yForce;
            std::tie(xForce, yForce) = tree->calcForceOn(&bodies[i], theta);
            forces[i].first = (xForce);
            forces[i].second = (yForce);
        }
    }

    for (unsigned int i = 0; i < bodies.size(); i++) {
        if (bodies[i].m > 0) {
            calcNewPos(&bodies[i], dt, forces[i].first, forces[i].second);
        }
    }
}


int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);
    double start = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    options_t opts;

    // std::vector<Body *>bodies;
    std::vector<Body> bodies;
    double theta;
    double dt; 
    int steps;
    int totalNumBodies;
    if(rank == 0) {
        get_opts(argc, argv, &opts);
        theta = opts.theta;
        dt = opts.timeStep;
        steps = opts.steps;
        readFile(opts.inputFileName, &opts, bodies);
        totalNumBodies = bodies.size();
    }

    GLFWwindow* window;
    if (rank == 0 && opts.visualize) {
        /* OpenGL window dims */
        int width=600, height=600;
        if( !glfwInit() ){
            fprintf( stderr, "Failed to initialize GLFW\n" );
            return -1;
        }
        // Open a window and create its OpenGL context
        window = glfwCreateWindow( width, height, "Simulation", NULL, NULL);
        if( window == NULL ){
            fprintf( stderr, "Failed to open GLFW window.\n" );
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window); // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            fprintf(stderr, "Failed to initialize GLEW\n");
            return -1;
        }
        // Ensure we can capture the escape key being pressed below
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    }

    MPI_Bcast(&theta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&dt, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&totalNumBodies, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        bodies.resize(totalNumBodies);
    }

    // define bodies struct
    const int numItems = 6;
    int blockLen[6] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype types[6] = { MPI_INT, MPI_DOUBLE, 
            MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, 
            MPI_DOUBLE };
    MPI_Datatype mpiBody;
    MPI_Aint offsets[6] = {
            offsetof(Body, index),
            offsetof(Body, x),
            offsetof(Body, y),
            offsetof(Body, m),
            offsetof(Body, vx),
            offsetof(Body, vy),
            // offsetof(Body, fx),
            // offsetof(Body, fy)
    };

    MPI_Type_create_struct(numItems, blockLen, offsets, types, &mpiBody);
    MPI_Type_commit(&mpiBody);
    for (int i = 0; i < steps; i++) {
        MPI_Bcast(&bodies[0], bodies.size(), mpiBody, 0, MPI_COMM_WORLD);
        double treeTime = MPI_Wtime();

        QuadTree *tree = new QuadTree(4.0, 4.0);
        for (unsigned int j = 0; j < bodies.size(); j++) {
            //std::cout << "inserting " << bodies[j]->getIndex() << " into tree" << std::endl;
            tree->insert(&bodies[j]);
        }

        treeTime = MPI_Wtime() - treeTime;
        // if(rank == 0)
        // std::cout << "tree time: " << treeTime << ", ";

        if(size == 1) {
            double runTime = MPI_Wtime();
            run(tree, bodies, opts.theta, opts.timeStep);
            runTime = MPI_Wtime() - runTime;
            // std::cout << "run time: " << runTime << std::endl;
        } else {
            // calculate which ones this process works on.
            std::vector<unsigned int> indices;
            int nBodies = bodies.size(); 
            for(int curr = rank; curr < nBodies; curr += size) {
                indices.push_back(curr);
            }
            std::vector<std::pair<double, double>> forces(indices.size());
            double runTime = MPI_Wtime();
            for (unsigned int j = 0; j < indices.size(); j++){
                if (bodies[indices[j]].m > 0) {
                    double xForce;
                    double yForce;
                    std::tie(xForce, yForce) = tree->calcForceOn(&bodies[indices[j]], theta);
                    forces[j].first = (xForce);
                    forces[j].second = (yForce);
                }
            }
            runTime = MPI_Wtime() - runTime;
            // if(rank == 0)
            // std::cout << "runtime: " << runTime << ", ";
            for (unsigned int j = 0; j < indices.size(); j++) {
                if (bodies[indices[j]].m > 0) {
                    calcNewPos(&bodies[indices[j]], dt, forces[j].first, forces[j].second);
                }
            }
            if ( rank != 0) {
                //std::cout << "size is " << indices.size() << std::endl;
                for (unsigned int j = 0; j < indices.size(); j++) {
                    MPI_Send(&bodies[indices[j]], 1, mpiBody, 0, 0, MPI_COMM_WORLD);
                }
            } else {
                int numReceives = bodies.size() - indices.size();
                // MPI_Status status;
                //std::cout << "size: " << bodies.size() << " trying to receive " << numReceives << std::endl;
                double recvTime = MPI_Wtime();
                Body *temp = (Body *)malloc(sizeof(Body));
                for(int j = 0; j < numReceives; j++) {
                    MPI_Recv(temp, 1, mpiBody, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    int index = temp->index;
                    copy(*temp, bodies[index]); 
                }
                free(temp);
                recvTime = MPI_Wtime() - recvTime;
                // std::cout << "recvTime " << recvTime << std::endl;
            }
        }

        if(rank == 0 && opts.visualize) {
            glClear( GL_COLOR_BUFFER_BIT );
            drawOctreeBounds2D(tree);
            for(unsigned int p = 0; p < bodies.size(); p++)
                drawParticle2D(bodies[p].x, bodies[p].y, 0.01);
            // Swap buffers
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        tree->~QuadTree();
    }
    if(rank == 0) {
        double end = MPI_Wtime() - start;
        std::cout << end << std::endl;
        write_file(&opts, bodies);
    }
    MPI_Finalize();
    return 0;
}

// mpirun -np 1 valgrind --leak-check=yes ./nbody -i input/nb-100.txt -s 20000 -t 0.5 -d 0.005 -o nb-100-2p
// mpirun -np 1 lldb -s autorun -- ./nbody -i input/nb-100.txt -s 20000 -t 0.5 -d 0.005 -o nb-100-2p
