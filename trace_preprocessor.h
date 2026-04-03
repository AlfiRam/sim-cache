#ifndef TRACE_PREPROCESSOR_H
#define TRACE_PREPROCESSOR_H

#include <cstdint>
#include <string>
#include <map>
#include <utility>

// scans entire trace file ahead of time so optimal replacement policy knows when each block will next be needed.
//
// returns map keyed by (block_address, access_index) whose value is
// absolute position of the next access to that block, or UINT64_MAX
// if block is never touched again.
std::map<std::pair<uint32_t, uint64_t>, uint64_t>
preprocess_trace_for_optimal(const std::string& trace_file, uint32_t offset_bits);

#endif 
