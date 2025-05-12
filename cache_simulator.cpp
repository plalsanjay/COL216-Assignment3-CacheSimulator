#include "cache_simulator.h"
#include <cstdlib>
#include <ctime>
#include <random>

CacheSimulator::CacheSimulator(const std::string& app_name, int s, int E, int b, 
                               const std::string& output_file, int random_seed) : 
    app_name(app_name),
    output_filename(output_file),
    s_bits(s),
    assoc(E),
    b_bits(b),
    seed(random_seed) {
    
    // Initialize random seed for tie breaking
    if (seed == 0) {
        std::random_device rd;
        seed = rd();
    }
    std::srand(seed);
    
    // Create the bus
    bus = std::make_unique<Bus>();
    
    // Create caches and cores
    for (int i = 0; i < 4; i++) {  // Quad core
        caches.push_back(std::make_unique<Cache>(i, s, E, b, bus.get()));
        bus->addCache(caches[i].get());
        
        // Create core with its trace file
        std::string trace_filename = app_name + "_proc" + std::to_string(i) + ".trace";
        std::cout << trace_filename << std::endl;
        cores.push_back(std::make_unique<Core>(i, caches[i].get(), trace_filename));
    }
}

void CacheSimulator::run() {
    int current_cycle = 0;
    bool all_done = false;
    
    // Continue until all cores are done
    while (!all_done) {
        all_done = true;
        
        // Try to execute one instruction per core
        for (int i = 0; i < cores.size(); i++) {
            bool active = cores[i]->executeNextInstruction(current_cycle);
            if (active) {
                all_done = false;
            }
        }
        
        // Break if all cores are done
        if (all_done) break;
        
        current_cycle++;
    }
}

void CacheSimulator::outputResults() {
    std::ofstream outfile;
    if (!output_filename.empty()) {
        outfile.open(output_filename);
        if (!outfile.is_open()) {
            std::cerr << "Error: Could not open output file: " << output_filename << std::endl;
            return;
        }
    }
    
    // Output stream: either file or stdout
    std::ostream& out = output_filename.empty() ? std::cout : outfile;
    
    out << "Cache Simulator Results for " << app_name << "\n";
    out << "===================================\n";
    out << "Cache parameters:\n";
    out << "  Set bits (s): " << s_bits << " (Sets: " << (1 << s_bits) << ")\n";
    out << "  Associativity (E): " << assoc << "\n";
    out << "  Block bits (b): " << b_bits << " (Block size: " << (1 << b_bits) << " bytes)\n";
    out << "  Total cache size per core: " << ((1 << s_bits) * assoc * (1 << b_bits)) << " bytes\n";
    out << "  Random seed: " << seed << "\n\n";
    
    // Per-core statistics
    out << "Per-core Statistics:\n";
    out << "-------------------\n";
    out << std::setw(10) << "Core ID" 
        << std::setw(15) << "Read Instr" 
        << std::setw(15) << "Write Instr" 
        << std::setw(15) << "Total Instr" 
        << std::setw(15) << "Total Cycles" 
        << std::setw(15) << "Idle Cycles" 
        << std::setw(15) << "Miss Rate" 
        << std::setw(15) << "Evictions" 
        << std::setw(15) << "Writebacks" << "\n";
    
    for (int i = 0; i < cores.size(); i++) {
        out << std::setw(10) << i 
            << std::setw(15) << cores[i]->getReadCount() 
            << std::setw(15) << cores[i]->getWriteCount() 
            << std::setw(15) << cores[i]->getInstructionCount() 
            << std::setw(15) << cores[i]->getTotalCycles() 
            << std::setw(15) << cores[i]->getIdleCycles() 
            << std::setw(15) << std::fixed << std::setprecision(4) << cores[i]->getMissRate() 
            << std::setw(15) << cores[i]->getEvictions() 
            << std::setw(15) << cores[i]->getWritebacks() << "\n";
    }
    
    // Global statistics
    out << "\nGlobal Statistics:\n";
    out << "-----------------\n";
    out << "Invalidations on bus: " << bus->getInvalidations() << "\n";
    out << "Data traffic on bus: " << bus->getDataTraffic() << " bytes\n";
    out << "Maximum execution time: " << getMaxExecutionTime() << " cycles\n";
    std::cout << "DEBUG: Before outputting stats, bus->getInvalidations()=" 
          << bus->getInvalidations() << std::endl;
    if (!output_filename.empty()) {
        outfile.close();
    }

}

int CacheSimulator::getMaxExecutionTime() {
    int max_time = 0;
    for (int i = 0; i < cores.size(); i++) {
        // Total execution time includes both active and idle cycles
        int core_total_time = cores[i]->getTotalCycles() + cores[i]->getIdleCycles();
        max_time = std::max(max_time, core_total_time);
    }
    return max_time;
}

