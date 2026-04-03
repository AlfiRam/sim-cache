# Cache Simulator

A cache and memory hierarchy simulator for CDA 5106. Simulates L1-only or L1+L2 cache configurations with configurable replacement policies and inclusion properties.

## Features

- L1-only or L1+L2 cache hierarchies
- Replacement policies: LRU, FIFO, Optimal
- Write-Back + Write-Allocate (WBWA) policy
- Configurable inclusive/non-inclusive caches
- Tracks miss rates, writebacks, and memory traffic

## Build

```bash
make
make clean
```

## Usage

```bash
./sim_cache <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>
```

**Parameters:**
- `BLOCKSIZE`: Block size in bytes
- `L1_SIZE`: L1 cache size in bytes
- `L1_ASSOC`: L1 associativity (1 = direct-mapped)
- `L2_SIZE`: L2 cache size in bytes (0 = no L2)
- `L2_ASSOC`: L2 associativity
- `REPLACEMENT_POLICY`: 0=LRU, 1=FIFO, 2=Optimal
- `INCLUSION_PROPERTY`: 0=non-inclusive, 1=inclusive
- `trace_file`: Path to trace file

**Examples:**

L1-only with LRU:
```bash
./sim_cache 16 1024 2 0 0 0 0 traces/gcc_trace.txt
```

L1+L2 inclusive hierarchy:
```bash
./sim_cache 16 1024 2 8192 4 0 1 traces/gcc_trace.txt
```

## Validation

Validate against reference outputs:
```bash
./sim_cache 16 1024 2 0 0 0 0 traces/gcc_trace.txt > output.txt
diff -iw output.txt validation_runs/validation0.txt
```

## Experiment Runner

Run all experiments for the report:
```bash
./run_experiments.sh
```

This script runs 94 simulations using the GCC trace and saves results to `experiment_results.csv`:
- **Graph 1 & 2** (55 simulations): L1-only, LRU, varying size and associativity
- **Graph 3** (27 simulations): L1-only, 4-way, varying size and replacement policy
- **Graph 4** (12 simulations): L1+L2, varying L2 size and inclusion property

## Project Structure

- `cache.cc/h` - Cache implementation
- `sim_cache.cc` - Main driver
- `trace_preprocessor.cc/h` - Optimal policy preprocessing
- `traces/` - SPEC2000 benchmark traces
- `validation_runs/` - Reference outputs
- `debug_runs/` - Debug traces