#include "argparse.h"

void get_opts(int argc,
              char **argv,
              options_t *opts)
{
    if (argc == 1)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t-i <input_file>" << std::endl;
        std::cout << "\t-o <output_file>" << std::endl;
        std::cout << "\t-s <steps>" << std::endl;
        std::cout << "\t-t <theta>" << std::endl;
        std::cout << "\t-d <dt/time_step>" << std::endl;
        std::cout << "\t-v <visualize_flag>" << std::endl;
        exit(0);
    }
    opts->visualize = false;

    int c;
    while ((c = getopt(argc, argv, "i:o:s:t:d:v")) != -1)
    {
        switch (c)
        {
        case 0:
            break;
        case 'i':
            opts->inputFileName = optarg;
            break;
        case 'o':
            opts->outputFileName = optarg;
            break;
        case 's':
            opts->steps = atoi((char *)optarg);
            break;
        case 't':
            opts->theta = std::strtod((char *)optarg, NULL);
            break;
        case 'd':
            opts->timeStep = std::strtod((char *)optarg, NULL);
            break;
        case 'v':
            opts->visualize = true;
            break;
        case ':':
            std::cerr << argv[0] << ": option -" << (char)optopt << "requires an argument." << std::endl;
            exit(1);
        }
    }
}
