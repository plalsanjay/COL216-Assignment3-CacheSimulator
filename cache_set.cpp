#include "cache_simulator.h"

CacheSet::CacheSet(int E, int B) : associativity(E), block_size(B) {
    for (int i = 0; i < associativity; i++) {  // Use associativity here instead of E
        lines.push_back(std::make_unique<CacheLine>(block_size));  // Use block_size here instead of B
    }
}

CacheLine* CacheSet::findLine(uint32_t tag) {
    for (auto& line : lines) {
        if (line->valid && line->tag == tag) {
            return line.get();
        }
    }
    return nullptr;
}

CacheLine* CacheSet::findReplacementLine(int& eviction_result) {
    // First, check for any invalid lines
    for (int i = 0; i < associativity; i++) {  // Use associativity here
        if (!lines[i]->valid) {
            eviction_result = 0;  // No eviction needed
            return lines[i].get();
        }
    }
    
    // If all valid, find the LRU line
    int min_access = lines[0]->last_access;
    CacheLine* lru_line = lines[0].get();
    
    for (auto& line : lines) {
        if (line->last_access < min_access) {
            min_access = line->last_access;
            lru_line = line.get();
        }
    }
    
    // Check if the line to be evicted is dirty, which requires a writeback
    eviction_result = (lru_line->dirty) ? 2 : 1;  // 2 = dirty eviction, 1 = clean eviction
    
    return lru_line;
}

void CacheSet::updateLRU(CacheLine* line, int cycle) {
    line->last_access = cycle;
}
