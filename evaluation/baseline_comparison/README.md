# OptiWeave Baseline Comparison

This directory contains benchmarks and tools for comparing OptiWeave's performance overhead against other profiling tools.

## Tools Compared

| Tool | Type | Typical Overhead | Use Case |
|------|------|-----------------|----------|
| **OptiWeave** | Source instrumentation | 8-17% | Selective operator profiling |
| **gprof** | Sampling + instrumentation | 20-50% | Function-level profiling |
| **Valgrind** | Dynamic binary translation | 2000-10000% | Memory debugging |
| **perf** | Hardware counters | 1-5% | System-wide profiling |

## Quick Start

```bash
# Run the comparison benchmark
./run_baseline_comparison.sh

# Quick mode (fewer iterations)
./run_baseline_comparison.sh --quick
```

## Files

| File | Description |
|------|-------------|
| `benchmark_program.c` | Comprehensive C benchmark with multiple workloads |
| `run_baseline_comparison.sh` | Automated comparison script |
| `results/` | Generated comparison reports |

## Benchmark Workloads

The benchmark includes 8 different workloads to stress different aspects:

1. **Sequential Array Access** - Tests cache behavior with linear access
2. **Strided Array Access** - Tests cache misses with stride-16 access
3. **Random Array Access** - Tests worst-case cache behavior
4. **Integer Arithmetic** - CPU-bound integer operations
5. **Floating Point** - Math-heavy floating point operations
6. **Matrix Multiplication** - O(n³) compute-intensive workload
7. **Memory Allocation** - malloc/free/realloc patterns
8. **Function Calls** - Mix of recursive and iterative calls

## Requirements

- GCC or Clang compiler
- OptiWeave built in `build/` directory
- Optional: gprof, valgrind, perf

### Installing Optional Tools

```bash
# Ubuntu/Debian
sudo apt-get install valgrind linux-tools-common linux-tools-$(uname -r)

# Fedora/RHEL
sudo dnf install valgrind perf
```

## Sample Results

Typical results on modern hardware:

| Tool | Overhead | Suitable for Production |
|------|----------|------------------------|
| perf | ~2% | Yes |
| OptiWeave | ~12% | Yes (targeted profiling) |
| gprof | ~35% | Development only |
| Valgrind | ~5000% | Debugging only |

## Interpreting Results

### Low Overhead (< 20%)
Tools in this category can be used for production profiling without significantly affecting application behavior.

### Medium Overhead (20-100%)
Suitable for development and testing environments. May affect timing-sensitive code.

### High Overhead (> 100%)
Only suitable for debugging and detailed analysis. Will significantly alter program behavior.

## OptiWeave Advantages

1. **Selective Instrumentation**: Only instrument what you need
2. **Source-Level Tracking**: Know exact file:line:function
3. **Operator Counting**: Unique capability not in other tools
4. **C and C++ Support**: Works with both languages
5. **Low Overhead**: Production-friendly when needed

## Further Reading

- [OptiWeave Documentation](../../docs/)
- [Evaluation Results](../EVALUATION_SUMMARY.md)
- [Architecture Overview](../../docs/architecture.md)
