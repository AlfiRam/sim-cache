#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include "cache.h"
#include "trace_preprocessor.h"

using namespace std;

int main(int argc, char* argv[]) {
    // check argument count
    if (argc != 9) {
        cerr << "Error: Incorrect number of arguments" << endl;
        cerr << "Usage: " << argv[0] << " <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>" << endl;
        return 1;
    }

    // parse command-line arguments
    uint32_t blocksize = atoi(argv[1]);
    uint32_t l1_size = atoi(argv[2]);
    uint32_t l1_assoc = atoi(argv[3]);
    uint32_t l2_size = atoi(argv[4]);
    uint32_t l2_assoc = atoi(argv[5]);
    uint32_t replacement_policy = atoi(argv[6]);
    uint32_t inclusion_property = atoi(argv[7]);
    string trace_file = argv[8];

    // extract filename from path for display
    string trace_filename = trace_file;
    size_t last_slash = trace_file.find_last_of("/\\");
    if (last_slash != string::npos) {
        trace_filename = trace_file.substr(last_slash + 1);
    }

    // convert replacement policy number to enum
    ReplacementPolicy policy;
    if (replacement_policy == 0) policy = LRU;
    else if (replacement_policy == 1) policy = FIFO;
    else if (replacement_policy == 2) policy = OPTIMAL;
    else {
        cerr << "Error: Invalid replacement policy: " << replacement_policy << endl;
        return 1;
    }

    // for optimal policy, preprocess trace
    map<pair<uint32_t, uint64_t>, uint64_t> optimal_map;
    if (policy == OPTIMAL) {
        uint32_t offset_bits = (uint32_t)log2(blocksize);
        optimal_map = preprocess_trace_for_optimal(trace_file, offset_bits);
    }

    // caches inclusive?
    bool is_inclusive = (inclusion_property == 1);

    // create L1 cache
    Cache l1_cache(blocksize, l1_size, l1_assoc, policy, false);

    // set optimal map if using optimal policy
    if (policy == OPTIMAL) {
        l1_cache.set_optimal_map(optimal_map);
    }

    // create L2 cache (if needed)
    Cache* l2_cache = nullptr;
    if (l2_size > 0) {
        // L2 is inclusive if inclusion_property is set
        l2_cache = new Cache(blocksize, l2_size, l2_assoc, policy, is_inclusive);

        // set optimal map for L2 if using optimal 
        if (policy == OPTIMAL) {
            l2_cache->set_optimal_map(optimal_map);
        }

        // link L1 to L2 and L2 to L1
        l1_cache.set_next_level(l2_cache);
        if (is_inclusive) {
            l2_cache->set_inner_level(&l1_cache);
        }
    }

    // open trace file
    ifstream trace(trace_file);
    if (!trace.is_open()) {
        cerr << "Error: Could not open trace file " << trace_file << endl;
        return 1;
    }

    // process trace
    string operation;
    uint32_t address;
    while (trace >> operation >> hex >> address) {
        bool is_write = (operation == "w");
        l1_cache.access(address, is_write);
    }
    trace.close();

    // print sim config
    cout << "===== Simulator configuration =====" << endl;
    cout << "BLOCKSIZE:" << setw(14) << blocksize << endl;
    cout << "L1_SIZE:" << setw(16) << l1_size << endl;
    cout << "L1_ASSOC:" << setw(15) << l1_assoc << endl;
    cout << "L2_SIZE:" << setw(16) << l2_size << endl;
    cout << "L2_ASSOC:" << setw(15) << l2_assoc << endl;
    cout << "REPLACEMENT POLICY:" << setw(5);
    if (replacement_policy == 0) cout << "LRU";
    else if (replacement_policy == 1) cout << "FIFO";
    else cout << "optimal";
    cout << endl;
    cout << "INCLUSION PROPERTY:" << setw(5);
    cout << (inclusion_property == 0 ? "non-inclusive" : "inclusive");
    cout << endl;
    cout << "trace_file:" << setw(13) << trace_filename << endl;

    // print L1 contents
    cout << "===== L1 contents =====" << endl;
    l1_cache.print_contents();

    // print L2 contents (if exists)
    if (l2_cache != nullptr) {
        cout << "===== L2 contents =====" << endl;
        l2_cache->print_contents();
    }

    // print sim results
    cout << "===== Simulation results (raw) =====" << endl;
    cout << "a. number of L1 reads:" << setw(9) << l1_cache.get_reads() << endl;
    cout << "b. number of L1 read misses:" << setw(3) << l1_cache.get_read_misses() << endl;
    cout << "c. number of L1 writes:" << setw(8) << l1_cache.get_writes() << endl;
    cout << "d. number of L1 write misses:" << setw(2) << l1_cache.get_write_misses() << endl;
    cout << "e. L1 miss rate:" << setw(15) << fixed << setprecision(6) << l1_cache.get_miss_rate() << endl;
    cout << "f. number of L1 writebacks:" << setw(4) << l1_cache.get_writebacks() << endl;

    // L2 stats
    uint64_t l2_reads = 0;
    uint64_t l2_read_misses = 0;
    uint64_t l2_writes = 0;
    uint64_t l2_write_misses = 0;
    uint64_t l2_writebacks = 0;
    double l2_miss_rate = 0.0;

    if (l2_cache != nullptr) {
        l2_reads = l2_cache->get_reads();
        l2_read_misses = l2_cache->get_read_misses();
        l2_writes = l2_cache->get_writes();
        l2_write_misses = l2_cache->get_write_misses();
        l2_writebacks = l2_cache->get_writebacks();

        // L2 miss rate
        if (l2_reads > 0) {
            l2_miss_rate = (double)l2_read_misses / (double)l2_reads;
        }
    }

    cout << "g. number of L2 reads:" << setw(9) << l2_reads << endl;
    cout << "h. number of L2 read misses:" << setw(3) << l2_read_misses << endl;
    cout << "i. number of L2 writes:" << setw(8) << l2_writes << endl;
    cout << "j. number of L2 write misses:" << setw(2) << l2_write_misses << endl;
    if (l2_cache != nullptr) {
        cout << "k. L2 miss rate:" << setw(15) << fixed << setprecision(6) << l2_miss_rate << endl;
    } else {
        cout << "k. L2 miss rate:" << setw(15) << 0 << endl;
    }
    cout << "l. number of L2 writebacks:" << setw(4) << l2_writebacks << endl;

    // total memory traffic
    uint64_t memory_traffic;
    if (l2_cache != nullptr) {
        // with L2: memory traffic = L2 read misses + L2 write misses + L2 writebacks
        memory_traffic = l2_read_misses + l2_write_misses + l2_writebacks;

        // for inclusive caches: add L1->memory writebacks from back-invalidation
        if (is_inclusive) {
            memory_traffic += l1_cache.get_back_invalidation_writebacks();
        }
    } else {
        // without L2: memory traffic = L1 read misses + L1 write misses + L1 writebacks
        memory_traffic = l1_cache.get_read_misses() + l1_cache.get_write_misses() + l1_cache.get_writebacks();
    }
    cout << "m. total memory traffic:" << setw(5) << memory_traffic << endl;

    // clean up
    if (l2_cache != nullptr) {
        delete l2_cache;
    }

    return 0;
}
