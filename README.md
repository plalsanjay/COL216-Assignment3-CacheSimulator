# COL216-Assignment3-CacheSimulator
Simulate L1 cache in C++ for quad core processors, with cache coherence support.

# L1 Cache Simulator

This project implements a simulation of L1 caches for quad-core processors with cache coherence support using the MESI protocol.

## Features

- Simulates four processor cores each with its own L1 data cache
- MESI cache coherence protocol implementation
- Write-back and write-allocate cache policy
- LRU replacement strategy
- Supports configurable cache parameters (sets, associativity, block size)
- Detailed statistics tracking for cache performance analysis

## Requirements

- C++14 compatible compiler (g++ recommended)
- Make

## Compilation

To compile the simulator, run:

```
make
```

This will create an executable named `L1simulate`.

## Usage

```
./L1simulate [options]
```

### Options

- `-t <tracefile>`: Name of parallel application (e.g. app1) whose 4 traces are to be used in simulation
- `-s <s>`: Number of set index bits (number of sets in the cache = S = 2^s)
- `-E <E>`: Associativity (number of cache lines per set)
- `-b <b>`: Number of block bits (block size = B = 2^b)
- `-o <outfilename>`: Logs output in file for plotting etc.
- `-h`: Prints help message

### Example

```
./L1simulate -t app1 -s 6 -E 2 -b 5 -o app1_results.txt
```

This command runs the simulation with:
- Trace files: app1_proc0.trace, app1_proc1.trace, app1_proc2.trace, app1_proc3.trace
- 2^6 = 64 sets in the cache
- 2-way set associative
- 2^5 = 32 byte blocks
- Output saved to app1_results.txt

## Expected Trace Format

Each trace file contains memory operations with the following format:
```
R 0x7e1ac04c
W 0x7e1afe78
```

Where:
- First column: 'R' for read operations, 'W' for write operations
- Second column: Memory address in hexadecimal format

## Implementation Details

### Core Classes

1. **CacheLine**: Represents a single cache line with MESI state
2. **CacheSet**: Collection of cache lines forming a set with LRU tracking
3. **Cache**: The L1 cache implementation for a processor core
4. **Core**: Represents a processor core that executes instructions
5. **Bus**: Shared bus between cores that implements the coherence protocol
6. **CacheSimulator**: Main simulation coordinator

### MESI Protocol Implementation

The simulator implements the full MESI (Modified, Exclusive, Shared, Invalid) protocol for cache coherence:

- **M (Modified)**: The cache line is only in this cache and has been modified
- **E (Exclusive)**: The cache line is only in this cache but matches memory
- **S (Shared)**: The cache line may be present in other caches
- **I (Invalid)**: The cache line is invalid

### Timing Model

The simulator implements the timing model as specified:
- L1 cache hit: 1 cycle
- Memory fetch: 100 cycles
- Cache-to-cache transfer: 2 cycles per word
- Evicting dirty blocks: 100 cycles

### Cache Policies

- **Write Policy**: Write-back, write-allocate
- **Replacement Policy**: LRU (Least Recently Used)

## Output Statistics

The simulator generates comprehensive statistics including:
1. Number of read/write instructions per core
2. Total execution cycles per core
3. Number of idle cycles per core
4. Data cache miss rate for each core
5. Number of cache evictions per core
6. Number of writebacks per core
7. Number of invalidations on the bus
8. Amount of data traffic on the bus
9. Maximum execution time across all cores

## Additional Notes

- Ties for bus transactions are broken arbitrarily
- Caches are blocking (core stalls on cache miss)
- Each memory reference accesses 32-bit (4-bytes) of data
