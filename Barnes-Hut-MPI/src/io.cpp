#include "io.h"
#include "vector"

void readFile(const char* fileName, options_t *options, std::vector<Body> &bodies){
    std::ifstream input;

    input.open(fileName);
    if (!input.is_open()) {
        std::cerr << "ERROR: Unable to open file" << std::endl; 
        return;
    }
    std::string line;
    getline(input, line);
    int numBodies = stoi(line);

    for (int i = 0; i < numBodies; i++){
        getline(input, line);
        Body temp;
        getFromString(temp, line);
        bodies.push_back(temp);
    }
    input.close();
}

void write_file(options_t *options, std::vector<Body> &bodies) {
  // Open file
	std::ofstream out;
	out.open(options->outputFileName, std::ofstream::trunc);
    unsigned int size = bodies.size();
    out << size << "\n";

	// Write solution to output file
	for (unsigned int i = 0; i < bodies.size(); ++i) {
		out << createLine(&bodies[i]);
	}

	out.flush();
	out.close();
	
	// free(opts->output_vals);
}
