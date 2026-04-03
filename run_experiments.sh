#!/bin/bash

# Script to run all cache simulator experiments for the report
# Uses gcc_trace.txt for all experiments

TRACE="traces/gcc_trace.txt"
OUTPUT_CSV="experiment_results.csv"
BLOCKSIZE=32

# Create CSV header
echo "graph,L1_size,L1_assoc,L2_size,L2_assoc,replacement_policy,inclusion_property,L1_reads,L1_writes,L1_read_misses,L1_write_misses,L1_miss_rate,L1_writebacks,L2_reads,L2_read_misses,L2_writes,L2_write_misses,L2_miss_rate,L2_writebacks,total_memory_traffic" > "$OUTPUT_CSV"

# Function to run simulation and parse output
run_sim() {
    local graph=$1
    local l1_size=$2
    local l1_assoc=$3
    local l2_size=$4
    local l2_assoc=$5
    local policy=$6
    local inclusive=$7

    echo "Running: Graph $graph, L1=$l1_size/$l1_assoc, L2=$l2_size/$l2_assoc, Policy=$policy, Inclusive=$inclusive"

    # Run simulator and capture output
    local output=$(./sim_cache $BLOCKSIZE $l1_size $l1_assoc $l2_size $l2_assoc $policy $inclusive "$TRACE")

    # Parse output - extract numbers after the colon
    local l1_reads=$(echo "$output" | grep "a. number of L1 reads:" | awk -F: '{print $2}' | xargs)
    local l1_read_misses=$(echo "$output" | grep "b. number of L1 read misses:" | awk -F: '{print $2}' | xargs)
    local l1_writes=$(echo "$output" | grep "c. number of L1 writes:" | awk -F: '{print $2}' | xargs)
    local l1_write_misses=$(echo "$output" | grep "d. number of L1 write misses:" | awk -F: '{print $2}' | xargs)
    local l1_miss_rate=$(echo "$output" | grep "e. L1 miss rate:" | awk -F: '{print $2}' | xargs)
    local l1_writebacks=$(echo "$output" | grep "f. number of L1 writebacks:" | awk -F: '{print $2}' | xargs)
    local l2_reads=$(echo "$output" | grep "g. number of L2 reads:" | awk -F: '{print $2}' | xargs)
    local l2_read_misses=$(echo "$output" | grep "h. number of L2 read misses:" | awk -F: '{print $2}' | xargs)
    local l2_writes=$(echo "$output" | grep "i. number of L2 writes:" | awk -F: '{print $2}' | xargs)
    local l2_write_misses=$(echo "$output" | grep "j. number of L2 write misses:" | awk -F: '{print $2}' | xargs)
    local l2_miss_rate=$(echo "$output" | grep "k. L2 miss rate:" | awk -F: '{print $2}' | xargs)
    local l2_writebacks=$(echo "$output" | grep "l. number of L2 writebacks:" | awk -F: '{print $2}' | xargs)
    local memory_traffic=$(echo "$output" | grep "m. total memory traffic:" | awk -F: '{print $2}' | xargs)

    # Append to CSV
    echo "$graph,$l1_size,$l1_assoc,$l2_size,$l2_assoc,$policy,$inclusive,$l1_reads,$l1_writes,$l1_read_misses,$l1_write_misses,$l1_miss_rate,$l1_writebacks,$l2_reads,$l2_read_misses,$l2_writes,$l2_write_misses,$l2_miss_rate,$l2_writebacks,$memory_traffic" >> "$OUTPUT_CSV"
}

echo "Starting experiments..."
echo ""

# Graph #1 and #2: L1-only, varying SIZE and ASSOC
echo "=== Graph 1 & 2: L1-only, LRU, varying size and associativity ==="
sizes=(1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)
for size in "${sizes[@]}"; do
    # Direct-mapped
    run_sim "1-2" $size 1 0 0 0 0

    # 2-way
    run_sim "1-2" $size 2 0 0 0 0

    # 4-way
    run_sim "1-2" $size 4 0 0 0 0

    # 8-way
    run_sim "1-2" $size 8 0 0 0 0

    # Fully-associative (SIZE/BLOCKSIZE)
    full_assoc=$((size / BLOCKSIZE))
    run_sim "1-2" $size $full_assoc 0 0 0 0
done

echo ""
echo "=== Graph 3: L1-only, 4-way, varying size and replacement policy ==="
sizes=(1024 2048 4096 8192 16384 32768 65536 131072 262144)
for size in "${sizes[@]}"; do
    # LRU
    run_sim "3" $size 4 0 0 0 0

    # FIFO
    run_sim "3" $size 4 0 0 1 0

    # Optimal
    run_sim "3" $size 4 0 0 2 0
done

echo ""
echo "=== Graph 4: L1+L2, varying L2 size and inclusion property ==="
l2_sizes=(2048 4096 8192 16384 32768 65536)
for l2_size in "${l2_sizes[@]}"; do
    # Non-inclusive
    run_sim "4" 1024 4 $l2_size 8 0 0

    # Inclusive
    run_sim "4" 1024 4 $l2_size 8 0 1
done

echo ""
echo "Experiments complete! Results saved to $OUTPUT_CSV"
echo "Total simulations: $(( $(wc -l < "$OUTPUT_CSV") - 1 ))"
