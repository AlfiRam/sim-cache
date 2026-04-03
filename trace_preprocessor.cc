#include "trace_preprocessor.h"
#include <fstream>
#include <vector>
#include <limits>

struct TraceEntry {
    char op;
    uint32_t block_address;
};

std::map<std::pair<uint32_t, uint64_t>, uint64_t>
preprocess_trace_for_optimal(const std::string& trace_file, uint32_t offset_bits) {
    std::map<std::pair<uint32_t, uint64_t>, uint64_t> next_use_map;
    std::vector<TraceEntry> trace;

    // read entire trace
    std::ifstream file(trace_file);
    std::string op_str;
    uint32_t address;

    while (file >> op_str >> std::hex >> address) {
        TraceEntry entry;
        entry.op = op_str[0];
        entry.block_address = address >> offset_bits;
        trace.push_back(entry);
    }
    file.close();

    // for each access, find the next time that block is accessed
    // store ABSOLUTE position
    for (uint64_t i = 0; i < trace.size(); i++) {
        uint32_t block_addr = trace[i].block_address;

        // search forward for next use of this block
        bool found = false;
        for (uint64_t j = i + 1; j < trace.size(); j++) {
            if (trace[j].block_address == block_addr) {
                // found next use at ABSOLUTE position j
                next_use_map[{block_addr, i}] = j;
                found = true;
                break;
            }
        }

        if (!found) {
            // never used again, use max value
            next_use_map[{block_addr, i}] = std::numeric_limits<uint64_t>::max();
        }
    }

    return next_use_map;
}
