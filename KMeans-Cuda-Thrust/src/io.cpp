#include "io.h"

void readFile(char* fileName, args_t *arguments){
    std::ifstream input;

    input.open(fileName);
    int n;
    std::string temp;
    std::getline(input, temp);
    std::istringstream tempSS(temp);
    tempSS >> n;
    arguments->nVals = n;
    arguments->input_vals = (float **) malloc(n * sizeof(float*));

    for(int i = 0; i < n; i++){
        arguments->input_vals[i] = (float *) malloc(arguments->dimension * sizeof(float));
        std::getline(input, temp);
        
        std::stringstream myStringStream(temp);

        // std::cout << "string stream is " << myStringStream.str() << std::endl;
        for(int j = -1; j < arguments->dimension; j++){
            if(j == -1) {
                // ignore first value of each line
                int nothing = 42;
                myStringStream >> nothing;
            }else {
                myStringStream >> arguments->input_vals[i][j];
            }
        }
        myStringStream.str("");
    }
}