# OptiWeave Baseline Comparison Results

## Test Environment
- **Date**: 2026-01-31 13:03:04
- **Platform**: Linux 6.8.0-59-generic
- **Benchmark**: simple_benchmark.cpp (array ops, arithmetic ops, mixed ops)
- **Iterations**: 10,000,000 per workload

## Raw Results (3 runs each, milliseconds)

| Tool | Run 1 | Run 2 | Run 3 | Average |
|------|-------|-------|-------|---------|
| **Baseline** (no profiling) | 673.67 | 701.43 | 699.76 | **691.62** |
| **OptiWeave** (source instrumentation) | 1158.73 | 1146.52 | 1206.32 | **1170.52** |
| **gprof** (sampling + instrumentation) | 747.44 | 733.76 | 813.68 | **764.96** |
| **perf stat** (hardware counters) | 790.51 | 647.33 | 788.47 | **742.10** |
| **Valgrind callgrind** (binary translation) | 7796.69 | - | - | **7796.69** |

## Overhead Comparison

| Tool | Average Time (ms) | Overhead (%) | Slowdown Factor |
|------|-------------------|--------------|-----------------|
| **Baseline** | 691.62 | 0% (reference) | 1.00x |
| **perf stat** | 742.10 | **+7.3%** | 1.07x |
| **gprof** | 764.96 | **+10.6%** | 1.11x |
| **OptiWeave** | 1170.52 | **+69.3%** | 1.69x |
| **Valgrind** | 7796.69 | **+1027.3%** | 11.27x |

## Analysis

### perf (7.3% overhead)
- Uses hardware performance monitoring units (PMUs)
- Minimal overhead because it's sampling-based
- Good for: Production profiling, system-wide analysis
- Limitation: Can't count specific operators

### gprof (10.6% overhead)
- Sampling + instrumentation at function boundaries
- Moderate overhead
- Good for: Function-level call graphs
- Limitation: No operator-level granularity

### OptiWeave (69.3% overhead)
- Source-to-source transformation
- Instruments every array access and arithmetic operation
- Good for: Operator counting, hotspot detection, bug detection
- What you get:
  - Array subscript counts (5 transformed in this benchmark)
  - Arithmetic operation counts (31 transformed)
  - Source location tracking (file:line:function)
  - Timing per operation

### Valgrind (1027% overhead / 11x slower)
- Full binary translation - every instruction simulated
- Complete memory tracking
- Good for: Memory leak detection, cache simulation
- Too slow for regular profiling

## Key Insight

**OptiWeave's overhead is HIGHER than perf/gprof because it does MORE:**

| Tool | What It Tracks | Granularity |
|------|----------------|-------------|
| perf | CPU cycles, cache misses | Sampled, statistical |
| gprof | Function calls, time | Function-level |
| **OptiWeave** | Every array[i], every +,-,*,/ | **Operator-level** |
| Valgrind | Every memory access | Instruction-level |

OptiWeave is the ONLY tool that can tell you:
- "This function has 1,234,567 array accesses"
- "This loop performs 890,123 multiplications"
- "The hottest line is file.cpp:42 with 82% of operations"

## When to Use Each Tool

| Scenario | Recommended Tool |
|----------|------------------|
| Production monitoring | perf |
| Function-level profiling | gprof |
| Operator counting & analysis | **OptiWeave** |
| Memory debugging | Valgrind |
| CI/CD bug detection | **OptiWeave** |
| Cache optimization | perf or OptiWeave |

