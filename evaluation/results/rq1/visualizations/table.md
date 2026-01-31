# RQ1 Overhead Results

| Benchmark | Baseline (s) | Instrumented (s) | Overhead (%) | Status |
|-----------|--------------|------------------|--------------|--------|
| gemm | 0.34 | 0.35 | **2.00%** | ✓ Excellent |
| gemver | 0.03 | 0.03 | **0.00%** | ✓ Excellent |
| jacobi-2d | 1 | 1.105 | **10.00%** | ⚠️ Good |
| nussinov | 2.31 | 2.82 | **22.00%** | ❌ High |
| 2mm | 3.32 | 3.285 | **-1.00%** | ✓ Excellent |
| correlation | 2.1 | 2.1 | **0.00%** | ✓ Excellent |
| heat-3d | 1.52 | 1.53 | **0.00%** | ✓ Excellent |
| atax | 0.02 | 0.02 | **0.00%** | ✓ Excellent |
| floyd-warshall | 15.56 | 15.69 | **0.00%** | ✓ Excellent |

## Summary

- **Mean overhead:** 3.67%
- **Median overhead:** 0.00%
- **Success rate:** 9/10 (90%)
- **Benchmarks < 5%:** 7/9 (78%)
