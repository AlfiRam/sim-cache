#ifndef CACHE_H
#define CACHE_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>

// replacement policies
enum ReplacementPolicy {
    LRU = 0,
    FIFO = 1,
    OPTIMAL = 2
};

// represents a single cache line
struct CacheLine {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint64_t lru_counter;      // for LRU 
    uint64_t fifo_counter;     // for FIFO 
    uint64_t optimal_next_use; // for Optimal 

    CacheLine() : valid(false), dirty(false), tag(0), lru_counter(0),
                  fifo_counter(0), optimal_next_use(0) {}
};

// result of a cache access
struct AccessResult {
    bool hit;
    bool writeback_needed;
    uint32_t writeback_address;  // block address to writeback

    AccessResult() : hit(false), writeback_needed(false), writeback_address(0) {}
};

class Cache {
private:
    // config
    uint32_t blocksize;
    uint32_t size;
    uint32_t assoc;
    ReplacementPolicy policy;
    bool inclusive;

    // computed parameters
    uint32_t num_sets;
    uint32_t offset_bits;
    uint32_t index_bits;
    uint32_t tag_bits;

    // sets x ways
    std::vector<std::vector<CacheLine>> cache_lines;

    // stats
    uint64_t reads;
    uint64_t read_misses;
    uint64_t writes;
    uint64_t write_misses;
    uint64_t writebacks;
    uint64_t back_invalidation_writebacks; 

    // replacement policy counters
    uint64_t global_lru_counter;
    uint64_t global_fifo_counter;

    // Optimal policy: map of (block_address, access_index) -> next_use_distance
    std::map<std::pair<uint32_t, uint64_t>, uint64_t> optimal_next_use_map;
    uint64_t current_access_index;

    // hierarchy support
    Cache* next_level;
    Cache* inner_level;  // for inclusive: L2 points to L1

    // helpers
    void decompose_address(uint32_t address, uint32_t& tag, uint32_t& index, uint32_t& offset);
    uint32_t get_block_address(uint32_t tag, uint32_t index);
    int find_line_in_set(uint32_t set_index, uint32_t tag);
    int find_invalid_line(uint32_t set_index);
    int find_lru_victim(uint32_t set_index);
    int find_fifo_victim(uint32_t set_index);
    int find_optimal_victim(uint32_t set_index);

public:
    Cache(uint32_t blocksize, uint32_t size, uint32_t assoc, ReplacementPolicy policy, bool inclusive = false);

    // main access function
    AccessResult access(uint32_t address, bool is_write);

    // for Optimal: set next-use map before simulation
    void set_optimal_map(const std::map<std::pair<uint32_t, uint64_t>, uint64_t>& map);

    // for hierarchy: set next level cache
    void set_next_level(Cache* next) { next_level = next; }
    void set_inner_level(Cache* inner) { inner_level = inner; }

    // for inclusive: back-invalidate a block in inner level
    // returns number of writebacks to memory (1 if block was dirty, 0 not)
    uint64_t back_invalidate(uint32_t address);

    // stats
    uint64_t get_reads() const { return reads; }
    uint64_t get_read_misses() const { return read_misses; }
    uint64_t get_writes() const { return writes; }
    uint64_t get_write_misses() const { return write_misses; }
    uint64_t get_writebacks() const { return writebacks; }
    uint64_t get_back_invalidation_writebacks() const { return back_invalidation_writebacks; }
    double get_miss_rate() const;

    // for printing cache contents
    void print_contents() const;

    // getters for config
    uint32_t get_num_sets() const { return num_sets; }
    uint32_t get_assoc() const { return assoc; }
};

#endif
