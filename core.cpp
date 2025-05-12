#include "cache_simulator.h"
#include <sstream>

Core::Core(int id, Cache* cache, const std::string& trace_filename) : 
    id(id), 
    cache(cache), 
    total_cycles(0),
    idle_cycles(0),
    instruction_count(0),
    is_stalled(false),
    stall_until_cycle(0) {
    
    trace_file.open(trace_filename);
    if (!trace_file.is_open()) {
        std::cerr << "Error: Could not open trace file: " << trace_filename << std::endl;
        exit(1);
    }
}

Core::~Core() {
    if (trace_file.is_open()) {
        trace_file.close();
    }
}

bool Core::hasMoreInstructions() {
    return trace_file.good() && !trace_file.eof();
}

bool Core::executeNextInstruction(int current_cycle) {
    // If stalled, just update idle cycles and return true (still active)
    if (is_stalled && current_cycle < stall_until_cycle) {
        idle_cycles++;
        return true;  // Still active but stalled
    }
    
    is_stalled = false;
    
    // Check if there are more instructions in the trace file
    if (trace_file.eof() || !trace_file.good()) {
        return false;  // No more instructions, core is done
    }
    
    // Read the next instruction
    std::string line;
    std::getline(trace_file, line);
    
    // Skip empty lines and try again
    if (line.empty()) {
        return true;  // Skip empty line but stay active
    }
    
    // Parse instruction
    char op;
    std::string addr_str;
    std::stringstream ss(line);
    ss >> op >> addr_str;
    
    // Check if parsing succeeded
    if (ss.fail()) {
        return true;  // Skip malformed lines but stay active
    }
    
    // Convert hexadecimal address string to uint32_t
    uint32_t addr;
    if (addr_str.substr(0, 2) == "0x") {
        addr_str = addr_str.substr(2);  // Remove "0x" prefix
    }
    addr = std::stoul(addr_str, nullptr, 16);
    
    // Update instruction count
    instruction_count++;
    
    // Execute the instruction
    int cycles_taken = 0;
    bool hit = false;
    
    if (op == 'R' || op == 'r') {
        hit = cache->read(addr, current_cycle, cycles_taken);
    } else if (op == 'W' || op == 'w') {
        hit = cache->write(addr, current_cycle, cycles_taken);
    } else {
        std::cerr << "Warning: Unknown operation type: " << op << std::endl;
        return true;  // Skip unknown operations but stay active
    }
    
    // Update cycle counts
    total_cycles++;  // Count only the current cycle in total_cycles
    
    if (!hit) {
        is_stalled = true;
        stall_until_cycle = current_cycle + cycles_taken;
        // Future cycles during stall will be counted as idle_cycles
    }
    
    // Always return true when we've processed an instruction
    // This keeps the core active in the simulation
    return true;
}