#include "cache.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>

Cache::Cache(uint32_t blocksize, uint32_t size, uint32_t assoc, ReplacementPolicy policy, bool inclusive)
    : blocksize(blocksize), size(size), assoc(assoc), policy(policy), inclusive(inclusive),
      reads(0), read_misses(0), writes(0), write_misses(0), writebacks(0),
      back_invalidation_writebacks(0),
      global_lru_counter(0), global_fifo_counter(0), current_access_index(0),
      next_level(nullptr), inner_level(nullptr) {

    // compute derived params
    num_sets = size / (assoc * blocksize);
    offset_bits = (uint32_t)log2(blocksize);
    index_bits = (uint32_t)log2(num_sets);
    tag_bits = 32 - offset_bits - index_bits;

    // init cache storage
    cache_lines.resize(num_sets);
    for (uint32_t i = 0; i < num_sets; i++) {
        cache_lines[i].resize(assoc);
    }
}

void Cache::decompose_address(uint32_t address, uint32_t& tag, uint32_t& index, uint32_t& offset) {
    offset = address & ((1 << offset_bits) - 1);
    uint32_t block_address = address >> offset_bits;
    index = block_address & ((1 << index_bits) - 1);
    tag = block_address >> index_bits;
}

uint32_t Cache::get_block_address(uint32_t tag, uint32_t index) {
    return (tag << index_bits) | index;
}

int Cache::find_line_in_set(uint32_t set_index, uint32_t tag) {
    for (uint32_t way = 0; way < assoc; way++) {
        if (cache_lines[set_index][way].valid &&
            cache_lines[set_index][way].tag == tag) {
            return way;
        }
    }
    return -1;
}

int Cache::find_invalid_line(uint32_t set_index) {
    for (uint32_t way = 0; way < assoc; way++) {
        if (!cache_lines[set_index][way].valid) {
            return way;
        }
    }
    return -1;
}

int Cache::find_lru_victim(uint32_t set_index) {
    int victim_way = 0;
    uint64_t min_counter = cache_lines[set_index][0].lru_counter;

    for (uint32_t way = 1; way < assoc; way++) {
        if (cache_lines[set_index][way].lru_counter < min_counter) {
            min_counter = cache_lines[set_index][way].lru_counter;
            victim_way = way;
        }
    }

    return victim_way;
}

int Cache::find_fifo_victim(uint32_t set_index) {
    int victim_way = 0;
    uint64_t min_counter = cache_lines[set_index][0].fifo_counter;

    for (uint32_t way = 1; way < assoc; way++) {
        if (cache_lines[set_index][way].fifo_counter < min_counter) {
            min_counter = cache_lines[set_index][way].fifo_counter;
            victim_way = way;
        }
    }

    return victim_way;
}

int Cache::find_optimal_victim(uint32_t set_index) {
    int victim_way = 0;
    uint64_t max_next_use = cache_lines[set_index][0].optimal_next_use;

    // find block with furthest next use (or never used again)
    for (uint32_t way = 1; way < assoc; way++) {
        if (cache_lines[set_index][way].optimal_next_use > max_next_use) {
            max_next_use = cache_lines[set_index][way].optimal_next_use;
            victim_way = way;
        }
    }

    return victim_way;
}

void Cache::set_optimal_map(const std::map<std::pair<uint32_t, uint64_t>, uint64_t>& map) {
    optimal_next_use_map = map;
}

AccessResult Cache::access(uint32_t address, bool is_write) {
    AccessResult result;

    // decompose address
    uint32_t tag, index, offset;
    decompose_address(address, tag, index, offset);
    uint32_t block_addr = get_block_address(tag, index);

    // update stats
    if (is_write) {
        writes++;
    } else {
        reads++;
    }

    // check for hit
    int hit_way = find_line_in_set(index, tag);

    if (hit_way != -1) {
        // HIT
        result.hit = true;

        // update replacement policy state based on policy type
        if (policy == LRU) {
            // LRU: update on both hit and miss
            global_lru_counter++;
            cache_lines[index][hit_way].lru_counter = global_lru_counter;
        } else if (policy == FIFO) {
            // FIFO: do NOT update on hit, only on insertion
        } else if (policy == OPTIMAL) {
            // Optimal: update next-use for current access
            auto it = optimal_next_use_map.find({block_addr, current_access_index});
            if (it != optimal_next_use_map.end()) {
                cache_lines[index][hit_way].optimal_next_use = it->second;
            } else {
                // never used again
                cache_lines[index][hit_way].optimal_next_use = std::numeric_limits<uint64_t>::max();
            }
        }

        // update dirty bit if write
        if (is_write) {
            cache_lines[index][hit_way].dirty = true;
        }
    } else {
        // MISS
        result.hit = false;

        if (is_write) {
            write_misses++;
        } else {
            read_misses++;
        }

        // find a place to put new block
        int victim_way = find_invalid_line(index);

        if (victim_way == -1) {
            // all ways are valid, need to evict
            if (policy == LRU) {
                victim_way = find_lru_victim(index);
            } else if (policy == FIFO) {
                victim_way = find_fifo_victim(index);
            } else if (policy == OPTIMAL) {
                victim_way = find_optimal_victim(index);
            }

            // for inclusive caches - back-invalidate victim from inner level
            if (inclusive && inner_level != nullptr) {
                uint32_t victim_tag = cache_lines[index][victim_way].tag;
                uint32_t victim_block_addr = get_block_address(victim_tag, index);
                uint32_t victim_byte_addr = victim_block_addr << offset_bits;
                inner_level->back_invalidate(victim_byte_addr);
            }

            // check if victim is dirty
            if (cache_lines[index][victim_way].dirty) {
                result.writeback_needed = true;
                uint32_t victim_tag = cache_lines[index][victim_way].tag;
                result.writeback_address = get_block_address(victim_tag, index);
                writebacks++;

                // if next level exists, write victim to next level
                if (next_level != nullptr) {
                    // Convert block address to byte address for next level
                    uint32_t victim_byte_addr = result.writeback_address << offset_bits;
                    next_level->access(victim_byte_addr, true);  // Write to next level
                }
            }
        }

        // if next level exists, read requested block from next level
        if (next_level != nullptr) {
            next_level->access(address, false);  // read from next level
        }

        // install new block
        cache_lines[index][victim_way].valid = true;
        cache_lines[index][victim_way].tag = tag;
        cache_lines[index][victim_way].dirty = is_write;

        // update replacement policy state based on policy type
        if (policy == LRU) {
            global_lru_counter++;
            cache_lines[index][victim_way].lru_counter = global_lru_counter;
        } else if (policy == FIFO) {
            // FIFO: update only on insertion
            global_fifo_counter++;
            cache_lines[index][victim_way].fifo_counter = global_fifo_counter;
        } else if (policy == OPTIMAL) {
            // Optimal: set next-use for newly inserted block
            auto it = optimal_next_use_map.find({block_addr, current_access_index});
            if (it != optimal_next_use_map.end()) {
                cache_lines[index][victim_way].optimal_next_use = it->second;
            } else {
                // never used again
                cache_lines[index][victim_way].optimal_next_use = std::numeric_limits<uint64_t>::max();
            }
        }
    }

    // increment access index for optimal policy
    current_access_index++;

    return result;
}

uint64_t Cache::back_invalidate(uint32_t address) {
    // back-invalidate block at given address
    // returns number of writebacks to memory (1 if dirty, 0 not)

    uint32_t tag, index, offset;
    decompose_address(address, tag, index, offset);

    // search for block in this cache
    int way = find_line_in_set(index, tag);

    if (way != -1) {
        // block found
        uint64_t wb_count = 0;

        // if dirty, count a writeback to main memory
        if (cache_lines[index][way].dirty) {
            wb_count = 1;
            back_invalidation_writebacks++;
        }

        // invalidate block
        cache_lines[index][way].valid = false;
        cache_lines[index][way].dirty = false;

        return wb_count;
    }

    return 0;  // block not found
}

double Cache::get_miss_rate() const {
    uint64_t total_accesses = reads + writes;
    if (total_accesses == 0) {
        return 0.0;
    }
    uint64_t total_misses = read_misses + write_misses;
    return (double)total_misses / (double)total_accesses;
}

void Cache::print_contents() const {
    for (uint32_t set = 0; set < num_sets; set++) {
        std::cout << "Set" << std::setw(6) << set << ":";

        // print valid blocks from way 0 to way assoc-1
        for (uint32_t way = 0; way < assoc; way++) {
            if (cache_lines[set][way].valid) {
                std::cout << std::setw(8) << std::hex << cache_lines[set][way].tag;
                if (cache_lines[set][way].dirty) {
                    std::cout << " D";
                }
                std::cout << "  ";
            }
        }
        std::cout << std::dec << std::endl;
    }
}
