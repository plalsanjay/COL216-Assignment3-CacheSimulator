#include "cache_simulator.h"

Cache::Cache(int core_id, int s, int E, int b, Bus* bus) : 
    core_id(core_id),
    assoc(E),
    s_bits(s), 
    sets(1 << s),
    block_size(1 << b),
    b_bits(b),
    bus(bus),
    read_count(0),
    write_count(0),
    read_misses(0),
    write_misses(0),
    evictions(0),
    writebacks(0) {
    
    // Initialize cache sets
    cache_sets.reserve(sets);
    for (int i = 0; i < sets; i++) {
        cache_sets.emplace_back(assoc, block_size);
    }
}

void Cache::extractAddressFields(uint32_t addr, uint32_t& tag, int& set_idx, uint32_t& block_offset) {
    block_offset = addr & ((1 << b_bits) - 1);
    set_idx = (addr >> b_bits) & ((1 << s_bits) - 1);
    tag = addr >> (b_bits + s_bits);
}

CacheLine* Cache::findLine(uint32_t addr, uint32_t& tag, int& set_idx) {
    uint32_t block_offset;
    extractAddressFields(addr, tag, set_idx, block_offset);
    return cache_sets[set_idx].findLine(tag);
}

bool Cache::read(uint32_t addr, int cycle, int& cycles_taken) {
    uint32_t tag;
    int set_idx;
    uint32_t block_offset;
    extractAddressFields(addr, tag, set_idx, block_offset);
    
    read_count++;
    CacheLine* line = cache_sets[set_idx].findLine(tag);
    
    if (line && line->valid && line->state != MESIState::INVALID) {
        // Cache hit
        cache_sets[set_idx].updateLRU(line, cycle);
        cycles_taken = 1;  // L1 hit takes 1 cycle
        return true;
    } else {
        // Cache miss
        read_misses++;
        
        // Fetch from memory or other caches
        int bus_cycles = 0;
        bus->processRead(core_id, addr, bus_cycles);
        
        // Find line to replace
        int eviction_result = 0;
        CacheLine* replacement = cache_sets[set_idx].findReplacementLine(eviction_result);
        
        // Handle eviction and writeback if necessary
        if (eviction_result > 0) {
            evictions++;
            if (eviction_result == 2) {  // Dirty eviction
                writebacks++;
                cycles_taken = 100;  // Writeback to memory takes 100 cycles
            } else {
                cycles_taken = 0;  // Clean eviction, no additional cycles
            }
        } else {
            cycles_taken = 0;  // No eviction
        }
        
        // Memory fetch or cache-to-cache transfer
        cycles_taken += (bus_cycles > 0) ? bus_cycles : 100;  // If no cache-to-cache, fetch from memory (100 cycles)
        
        // Update the line
        replacement->valid = true;
        replacement->tag = tag;
        replacement->state = MESIState::EXCLUSIVE;  // Initial state after read miss
        replacement->dirty = false;
        cache_sets[set_idx].updateLRU(replacement, cycle);
        
        return false;  // Cache miss
    }
}

bool Cache::write(uint32_t addr, int cycle, int& cycles_taken) {
    uint32_t tag;
    int set_idx;
    uint32_t block_offset;
    extractAddressFields(addr, tag, set_idx, block_offset);
    
    write_count++;
    CacheLine* line = cache_sets[set_idx].findLine(tag);
    
    if (line && line->valid && line->state != MESIState::INVALID) {
        // Cache hit, update based on current state
        cache_sets[set_idx].updateLRU(line, cycle);
        
        if (line->state == MESIState::MODIFIED) {
            // Already in Modified state, just update
            cycles_taken = 1;
        } else if (line->state == MESIState::EXCLUSIVE) {
            // Change from Exclusive to Modified
            line->state = MESIState::MODIFIED;
            line->dirty = true;
            cycles_taken = 1;
        } else if (line->state == MESIState::SHARED) {
            // Need to get exclusive ownership first
            int bus_cycles = 0;
            bus->processUpgrade(core_id, addr, bus_cycles);
            line->state = MESIState::MODIFIED;
            line->dirty = true;
            cycles_taken = 1 + bus_cycles;
        }
        
        return true;  // Cache hit
    } else {
        // Cache miss
        write_misses++;
        
        // Fetch from memory or other caches
        int bus_cycles = 0;
        bus->processWrite(core_id, addr, bus_cycles);
        
        // Find line to replace
        int eviction_result = 0;
        CacheLine* replacement = cache_sets[set_idx].findReplacementLine(eviction_result);
        
        // Handle eviction and writeback if necessary
        if (eviction_result > 0) {
            evictions++;
            if (eviction_result == 2) {  // Dirty eviction
                writebacks++;
                cycles_taken = 100;  // Writeback to memory takes 100 cycles
            } else {
                cycles_taken = 0;  // Clean eviction, no additional cycles
            }
        } else {
            cycles_taken = 0;  // No eviction
        }
        
        // Memory fetch or cache-to-cache transfer, plus upgrade to Modified
        cycles_taken += (bus_cycles > 0) ? bus_cycles : 100;  // If no cache-to-cache, fetch from memory (100 cycles)
        
        // Update the line
        replacement->valid = true;
        replacement->tag = tag;
        replacement->state = MESIState::MODIFIED;  // Initial state after write miss
        replacement->dirty = true;
        cache_sets[set_idx].updateLRU(replacement, cycle);
        
        return false;  // Cache miss
    }
    
}

// In cache.cpp
void Cache::busRead(uint32_t addr, Cache* requester, int& data_transfer_cycles) {
    uint32_t tag;
    int set_idx;
    uint32_t block_offset;
    extractAddressFields(addr, tag, set_idx, block_offset);
    
    CacheSet& set = cache_sets[set_idx];
    CacheLine* line = set.findLine(tag);
    
    // Debug what was found
    std::cout << "DEBUG: Core " << core_id << " busRead check for tag 0x" 
              << std::hex << tag << " set " << std::dec << set_idx;
    
    if (line && line->valid && line->state != MESIState::INVALID) {
        std::cout << " - FOUND in state " << (int)line->state << std::endl;
        
        if (line->state == MESIState::MODIFIED) {
            // Provide data and update state
            line->state = MESIState::SHARED;
            line->dirty = false;
            data_transfer_cycles = 2 * (block_size / 4);  // 2 cycles per word
            bus->addDataTraffic(block_size);
        } 
        else if (line->state == MESIState::EXCLUSIVE) {
            // Provide data and update state
            line->state = MESIState::SHARED;
            data_transfer_cycles = 2 * (block_size / 4);  // 2 cycles per word
            bus->addDataTraffic(block_size);
        }
        else if (line->state == MESIState::SHARED) {
            // Provide data (no state change needed)
            data_transfer_cycles = 2 * (block_size / 4);  // 2 cycles per word
            bus->addDataTraffic(block_size);
        }
    } else {
        std::cout << " - NOT FOUND" << std::endl;
    }
}

void Cache::busWrite(uint32_t addr, Cache* requester) {
    uint32_t tag;
    int set_idx;
    CacheLine* line = findLine(addr, tag, set_idx);
    
    if (line && line->valid && 
        (line->state == MESIState::SHARED || 
         line->state == MESIState::EXCLUSIVE)) {
        // Invalidate the line
        line->state = MESIState::INVALID;
        bus->incrementInvalidations();
    }
}

void Cache::busUpgrade(uint32_t addr) {
    uint32_t tag;
    int set_idx;
    CacheLine* line = findLine(addr, tag, set_idx);
    
    if (line && line->valid && line->state == MESIState::SHARED) {
        // Invalidate the line on upgrade request
        line->state = MESIState::INVALID;
        bus->incrementInvalidations();
    }
}

float Cache::getMissRate() const {
    int total_accesses = read_count + write_count;
    int total_misses = read_misses + write_misses;
    
    if (total_accesses == 0) return 0.0;
    return static_cast<float>(total_misses) / total_accesses;
}
