#!/bin/bash
# Compare baseline vs optimized cJSON performance

set -e

ITERATIONS=${1:-10000}

echo "======================================"
echo "OptiWeave Optimization Impact Test"
echo "======================================"
echo ""

# Build baseline version
echo "[1/4] Building BASELINE version..."
clang -O2 -Wall -Wno-deprecated-declarations -o benchmark_baseline \
    benchmark_single.c cJSON_baseline.c
echo "Done."
echo ""

# Build optimized version
echo "[2/4] Building OPTIMIZED version..."
clang -O2 -Wall -Wno-deprecated-declarations -o benchmark_optimized \
    benchmark_single.c cJSON_optimized.c
echo "Done."
echo ""

# Run baseline benchmark
echo "[3/4] Running BASELINE benchmark..."
echo "======================================"
./benchmark_baseline $ITERATIONS > /tmp/baseline_results.txt
cat /tmp/baseline_results.txt
echo ""

# Run optimized benchmark
echo "[4/4] Running OPTIMIZED benchmark..."
echo "======================================"
./benchmark_optimized $ITERATIONS > /tmp/optimized_results.txt
cat /tmp/optimized_results.txt
echo ""

# Extract and compare results
echo "======================================"
echo "PERFORMANCE COMPARISON"
echo "======================================"

baseline_avg=$(grep "Average time:" /tmp/baseline_results.txt | awk '{print $3}')
optimized_avg=$(grep "Average time:" /tmp/optimized_results.txt | awk '{print $3}')

echo "Baseline average:  $baseline_avg μs"
echo "Optimized average: $optimized_avg μs"
echo ""

# Calculate speedup using bc or awk
speedup=$(echo "scale=2; ($baseline_avg - $optimized_avg) / $baseline_avg * 100" | bc)
echo "Improvement: $speedup%"

if [ $(echo "$speedup > 0" | bc) -eq 1 ]; then
    echo "✓ Optimization SUCCESSFUL - code is faster!"
else
    echo "⚠ Optimization had negative or no impact"
fi

echo ""
echo "Speedup factor: $(echo "scale=2; $baseline_avg / $optimized_avg" | bc)x"
echo ""

# Cleanup
rm -f benchmark_baseline benchmark_optimized
