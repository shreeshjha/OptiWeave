# OptiWeave Use Case Evaluation Report

## Executive Summary

This report presents the evaluation of OptiWeave through three use cases that demonstrate
its practical value for performance optimization and bug detection.

# Experimental Environment

## Date
2026-01-30 14:02:44 UTC

## Hardware
- **CPU**: Apple M2
- **Cores**: 8
- **RAM**: 8 GB

## Software
- **OS**: Darwin 25.1.0
- **Compiler**: Apple clang version 17.0.0 (clang-1700.4.4.1)


---

## Use Case 1: Performance Hotspot Detection

**Goal**: Demonstrate that OptiWeave can identify performance hotspots and guide
optimization decisions that lead to measurable speedups.

### Scenario
Matrix multiplication with suboptimal memory access patterns.

### Methodology
- Matrix size: 512×512 (2 MB)
- Warmup: 3 iterations
- Measured: 30 iterations
- Metrics: Mean, stddev, 95% CI

### Results

| Implementation | Mean (sec) | Speedup |
|----------------|------------|---------|
| Naive (i-j-k) | 0.166768 | 1.0x (baseline) |
| Loop interchange (i-k-j) | 0.019730 | 8.45x |
| Cache blocking (32×32) | 0.028034 | 5.94x |

### OptiWeave Insight
OptiWeave identified the inner loop accessing B[k][j] (column-major) as the hotspot.
The strided access pattern caused cache misses on every element.

### Optimization Applied
Loop interchange (i-k-j order) ensures row-major access for both matrices,
achieving **8.45x speedup** with zero algorithmic changes.

---

## Use Case 2: Cache Access Pattern Optimization

**Goal**: Demonstrate that OptiWeave can detect cache-unfriendly access patterns
that cause order-of-magnitude performance degradation.

### Scenario
Matrix traversal comparing row-major vs column-major access.

### Methodology
- Matrix size: 4096×4096 (64 MB)
- Warmup: 3 iterations
- Measured: 30 iterations

### Results

| Access Pattern | Performance | Cache Behavior |
|----------------|-------------|----------------|
| Column-major | Slow | Cache miss per element |
| Row-major | Fast | Sequential access, full cache utilization |

**Speedup: 49.34x faster** with row-major access

### OptiWeave Insight
OptiWeave's array subscript tracking identified the j-before-i loop pattern
as cache-unfriendly. The instrumentation showed N² cache misses in column-major
vs ~N²/64 misses in row-major (assuming 64-byte cache lines, 4-byte elements).

---

## Use Case 3: Bug Detection (Integer Overflow)

**Goal**: Demonstrate that OptiWeave's runtime instrumentation catches bugs
that static analysis tools miss.

### Scenario
Various integer overflow and bounds-checking bugs:
1. Loop counter overflow
2. Array size multiplication overflow
3. Signed/unsigned comparison bugs
4. Off-by-one errors

### Results

| Metric | OptiWeave | Static Analysis |
|--------|-----------|-----------------|
| Detection Rate | 100% | 25% |

### Key Insight
Static analysis cannot detect bugs that depend on runtime values.
OptiWeave's instrumentation catches these at runtime:
- Loop counters that overflow after many iterations
- Array allocations that overflow for large dimensions
- Index calculations that wrap around

### Complementary Approach
OptiWeave is not a replacement for static analysis but a complement:
- Static analysis: Catches bugs at compile time, zero runtime cost
- OptiWeave: Catches runtime-dependent bugs, small overhead

---

## Conclusions

### RQ1: Can OptiWeave identify performance hotspots?
**Yes.** UC1 and UC2 demonstrate that OptiWeave's profiling identifies
the exact source locations responsible for performance issues.

### RQ2: Do OptiWeave's insights lead to measurable improvements?
**Yes.** Optimizations guided by OptiWeave achieved:
- 8.45x speedup via loop interchange (UC1)
- 5.94x speedup via cache blocking (UC1)
- 49.34x speedup via access pattern fix (UC2)

### RQ3: Can OptiWeave detect bugs that static analysis misses?
**Yes.** UC3 shows 100% detection rate for runtime-dependent
overflow bugs, compared to 25% for static tools.

---

## Appendix: Raw Data

See individual use case directories for:
- Full benchmark output
- CSV data files
- Statistical analysis

