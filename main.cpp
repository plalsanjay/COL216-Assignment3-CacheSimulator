#include "cache_simulator.h"
#include <cstring>

void printHelp() {
    std::cout << "Usage: ./L1simulate [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -t <tracefile>: name of parallel application (e.g. app1) whose 4 traces are to be used" << std::endl;
    std::cout << "  -s <s>: number of set index bits (number of sets in the cache = S = 2^s)" << std::endl;
    std::cout << "  -E <E>: associativity (number of cache lines per set)" << std::endl;
    std::cout << "  -b <b>: number of block bits (block size = B = 2^b)" << std::endl;
    std::cout << "  -o <outfilename>: logs output in file for plotting etc." << std::endl;
    std::cout << "  -h: prints this help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string app_name;
    int s = 0, E = 0, b = 0;
    std::string output_file;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            app_name = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            s = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-E") == 0 && i + 1 < argc) {
            E = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            b = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        }
    }
    
    // Check if required parameters are provided
    if (app_name.empty() || s <= 0 || E <= 0 || b <= 0) {
        std::cerr << "Error: Missing or invalid required parameters" << std::endl;
        printHelp();
        return 1;
    }
    
    // Create and run simulator
    CacheSimulator simulator(app_name, s, E, b, output_file);
    simulator.run();
    simulator.outputResults();
    
    return 0;
}
