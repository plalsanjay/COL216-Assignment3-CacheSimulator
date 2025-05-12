#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <memory>
#include <map>

// Forward declarations
class Cache;
class Core;
class Bus;

// MESI protocol states
enum class MESIState { MODIFIED, EXCLUSIVE, SHARED, INVALID };

// String representation of MESI states for debugging
inline std::string MESIStateToString(MESIState state) {
    switch (state) {
        case MESIState::MODIFIED: return "M";
        case MESIState::EXCLUSIVE: return "E";
        case MESIState::SHARED: return "S";
        case MESIState::INVALID: return "I";
        default: return "?";
    }
}

// Structure for a cache line
struct CacheLine {
    bool valid;
    uint32_t tag;
    MESIState state;
    std::unique_ptr<uint8_t[]> data;
    int last_access; // For LRU replacement
    bool dirty;      // For write-back policy
    
    CacheLine(int block_size) : 
        valid(false), 
        tag(0), 
        state(MESIState::INVALID), 
        data(new uint8_t[block_size]()), 
        last_access(0), 
        dirty(false) {}
};

// Structure for a cache set (contains E cache lines)
class CacheSet {
private:
    std::vector<std::unique_ptr<CacheLine>> lines;
    int associativity; // E
    int block_size;    // B
    
public:
    CacheSet(int E, int B);
    
    CacheLine* findLine(uint32_t tag);
    CacheLine* findReplacementLine(int& eviction_result);
    void updateLRU(CacheLine* line, int cycle);
};

// L1 Cache class
class Cache {
private:
    int core_id;    // Associated processor core ID
    int assoc;      // E
    int s_bits;     // s
    int sets;       // S = 2^s
    
    int block_size; // B = 2^b
    
    int b_bits;     // b
    
    std::vector<CacheSet> cache_sets;
    Bus* bus;       // Reference to the shared bus
    
    // Statistics
    int read_count;
    int write_count;
    int read_misses;
    int write_misses;
    int evictions;
    int writebacks;
    
public:
    Cache(int core_id, int s, int E, int b, Bus* bus);
    
    // Core operations
    bool read(uint32_t addr, int cycle, int& cycles_taken);
    bool write(uint32_t addr, int cycle, int& cycles_taken);
    
    // Bus snooping operations
    void busRead(uint32_t addr, Cache* requester, int& data_transfer_cycles);
    void busWrite(uint32_t addr, Cache* requester);
    void busUpgrade(uint32_t addr);
    
    // Helper methods
    void extractAddressFields(uint32_t addr, uint32_t& tag, int& set_idx, uint32_t& block_offset);
    CacheLine* findLine(uint32_t addr, uint32_t& tag, int& set_idx);
    
    // Statistics getters
    int getReadCount() const { return read_count; }
    int getWriteCount() const { return write_count; }
    float getMissRate() const; 
    int getEvictions() const { return evictions; }
    int getWritebacks() const { return writebacks; }
    int getCoreId() const { return core_id; }
};

// Processor core class
class Core {
private:
    int id;
    Cache* cache;
    std::ifstream trace_file;
    
    // Statistics
    int total_cycles;
    int idle_cycles;
    int instruction_count;
    bool is_stalled;
    int stall_until_cycle;
    
public:
    Core(int id, Cache* cache, const std::string& trace_filename);
    ~Core();
    int getId() const { return id; }
    bool executeNextInstruction(int current_cycle);
    bool hasMoreInstructions();
    
    // Statistics getters
    int getTotalCycles() const { return total_cycles; }
    int getIdleCycles() const { return idle_cycles; }
    int getInstructionCount() const { return instruction_count; }
    int getReadCount() const { return cache->getReadCount(); }
    int getWriteCount() const { return cache->getWriteCount(); }
    float getMissRate() const { return cache->getMissRate(); }
    int getEvictions() const { return cache->getEvictions(); }
    int getWritebacks() const { return cache->getWritebacks(); }
};

// Bus class for coherence
class Bus {
private:
    std::vector<Cache*> caches;
    int invalidations;
    int data_traffic_bytes;
    
public:
    Bus();
    
    void addCache(Cache* cache);
    void processRead(int requester_id, uint32_t addr, int& cycles_taken);
    void processWrite(int requester_id, uint32_t addr, int& cycles_taken);
    void processUpgrade(int requester_id, uint32_t addr, int& cycles_taken);
    
    // Statistics getters
    int getInvalidations() const ;
    int getDataTraffic() const { return data_traffic_bytes; }
    void incrementInvalidations() { invalidations++; }
    void addDataTraffic(int bytes);
};

// Main simulator class
class CacheSimulator {
private:
    std::vector<std::unique_ptr<Core>> cores;
    std::vector<std::unique_ptr<Cache>> caches;
    std::unique_ptr<Bus> bus;
    std::string app_name;
    std::string output_filename;
    int s_bits;  // Number of set index bits
    int assoc;   // Associativity
    int b_bits;  // Number of block bits
    
    // Random seed for tie-breaking
    int seed;
    
public:
    CacheSimulator(const std::string& app_name, int s, int E, int b, 
                   const std::string& output_file, int random_seed = 0);
    
    void run();
    void outputResults();
    int getMaxExecutionTime();
};

#endif // CACHE_SIMULATOR_H
