#include "argparse.h"

void get_opts(int argc,
              char **argv,
              options_t *opts)
{
    if (argc == 1)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t-k <num_cluster>" << std::endl;
        std::cout << "\t-d <dims>" << std::endl;
        std::cout << "\t-i <inputfilename>" << std::endl;
        std::cout << "\t-m <max_num_iter>" << std::endl;
        std::cout << "\t-t <threshold>" << std::endl;
        std::cout << "\t-c" << std::endl;
        std::cout << "\t-s <seed>" << std::endl;
        std::cout << "\t-u <Usage Mode>" << std::endl;
        exit(0);
    }
    opts->outputCentroids = false;

    int ind, c;
    while ((c = getopt(argc, argv, "k:d:i:m:t:cs:u:")) != -1)
    {
        switch (c)
        {
        case 0:
            break;
        case 'k':
            opts->num_cluster = atoi((char *)optarg);
            break;
        case 'd':
            opts->dims = atoi((char *)optarg);
            break;
        case 'i':
            opts->inputFileName = (char *)optarg;
            break;
        case 'm':
            opts->max_num_iter = atoi((char *)optarg);
            break;
        case 't':
            opts->threshold = std::strtod((char *)optarg, NULL);
            break;
        case 'c':
            opts->outputCentroids = true;
            break;
        case 's':
            opts->seed = atoi((char *)optarg);
            break;
        case 'u':
            opts->mode = atoi((char *)optarg);
            break;
        case ':':
            std::cerr << argv[0] << ": option -" << (char)optopt << "requires an argument." << std::endl;
            exit(1);
        }
    }
}
