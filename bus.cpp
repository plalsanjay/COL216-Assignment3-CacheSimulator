#include "cache_simulator.h"

Bus::Bus() : invalidations(0), data_traffic_bytes(0) {}

void Bus::addCache(Cache* cache) {
    caches.push_back(cache);
}

void Bus::addDataTraffic(int bytes) {
    // Debug print to verify this is being called
    //std::cout << "DEBUG: Adding " << bytes << " bytes to bus traffic" << std::endl;
    data_traffic_bytes += bytes;
}

// In bus.cpp
void Bus::processRead(int requester_id, uint32_t addr, int& cycles_taken) {
    // Debug the address being requested
    std::cout << "DEBUG: Core " << requester_id << " requesting address 0x" 
              << std::hex << addr << std::dec << std::endl;
    
    bool found_in_cache = false;
    int max_cycles = 0;
    
    // Check each cache in order (this ordering may affect which cache responds)
    for (Cache* cache : caches) {
        if (cache->getCoreId() != requester_id) {
            int data_transfer_cycles = 0;
            
            // Add debug before busRead call
            std::cout << "DEBUG: Checking if Core " << cache->getCoreId() 
                      << " has address 0x" << std::hex << addr << std::dec << std::endl;
            
            cache->busRead(addr, caches[requester_id], data_transfer_cycles);
            
            // Add debug after busRead call
            std::cout << "DEBUG: Core " << cache->getCoreId() 
                      << " data_transfer_cycles: " << data_transfer_cycles << std::endl;
            
            if (data_transfer_cycles > 0) {
                found_in_cache = true;
                max_cycles = std::max(max_cycles, data_transfer_cycles);
                
                // Debug what was found
                std::cout << "DEBUG: Found data in Core " << cache->getCoreId() 
                          << ", transfer will take " << data_transfer_cycles << " cycles" << std::endl;
            }
        }
    }
    
    // This is critical - make sure to set cycles correctly
    cycles_taken = found_in_cache ? max_cycles : 100;  // Memory fetch if not found
    
    std::cout << "DEBUG: Final cycles_taken: " << cycles_taken 
              << " (found_in_cache: " << found_in_cache << ")" << std::endl;
}

// In bus.cpp
void Bus::processWrite(int requester_id, uint32_t addr, int& cycles_taken) {
    // First, try to get data from another cache (same as read)
    bool found_in_cache = false;
    int max_cycles = 0;
    int invalidation_count = 0;
    // Check if any cache has the data to transfer
    for (Cache* cache : caches) {
        if (cache->getCoreId() != requester_id) {
            int data_transfer_cycles = 0;
            cache->busRead(addr, caches[requester_id], data_transfer_cycles);
            
            if (data_transfer_cycles > 0) {
                found_in_cache = true;
                max_cycles = std::max(max_cycles, data_transfer_cycles);
            }
        }
    }
    
    // Then invalidate all copies in other caches
    for (Cache* cache : caches) {
        if (cache->getCoreId() != requester_id) {
            cache->busWrite(addr, caches[requester_id]);
            std::cout << "DEBUG: Calling busWrite on Core " << cache->getCoreId() << std::endl;

            invalidation_count++;
        }
    }
    invalidations += invalidation_count;  // Update the counter
    std::cout << "DEBUG: After processWrite, invalidations=" << invalidations << std::endl;

    // Set cycles based on where data came from
    cycles_taken = found_in_cache ? max_cycles : 100;
}

void Bus::processUpgrade(int requester_id, uint32_t addr, int& cycles_taken) {
    // Invalidate SHARED copies in all other caches
    int invalidation_count = 0;
    for (Cache* cache : caches) {
        if (cache->getCoreId() != requester_id) {
            // Call busUpgrade without expecting a return value
            cache->busUpgrade(addr);
            
            // Assume an invalidation happens when we call busUpgrade
            // Again, this is slightly inaccurate but simpler
            invalidation_count++;
        }
    }
    
    invalidations += invalidation_count;  // Update the counter
    
    // Upgrade takes 2 cycles for bus transaction
    cycles_taken = 2;
}

// In Bus::getInvalidations:
int Bus::getInvalidations() const {
    std::cout << "DEBUG: getInvalidations called, returning " << invalidations << std::endl;
    return invalidations;
}
