#!/bin/bash
# Rigorous overhead measurement with proper statistical methodology

echo "Rigorous Overhead Measurement"
echo "=============================="
echo ""

# Configuration
SIZE=10000
ITERS=5000
RUNS=5

echo "Configuration:"
echo "  Array size: $SIZE elements"
echo "  Iterations: $ITERS"
echo "  Measurement runs: $RUNS"
echo ""

# Warmup
echo "Warming up..."
for i in {1..10}; do
    ./benchmark_baseline $SIZE $ITERS > /dev/null 2>&1
    ./benchmark_instrumented $SIZE $ITERS > /dev/null 2>&1
done
echo "Done."
echo ""

# Collect baseline times
echo "Collecting BASELINE measurements..."
baseline_times=()
for i in $(seq 1 $RUNS); do
    time_ms=$(./benchmark_baseline $SIZE $ITERS | grep "Total time:" | awk '{print $3}')
    baseline_times+=($time_ms)
    echo "  Run $i: $time_ms ms"
done
echo ""

# Collect instrumented times
echo "Collecting INSTRUMENTED measurements..."
instrumented_times=()
for i in $(seq 1 $RUNS); do
    time_ms=$(./benchmark_instrumented $SIZE $ITERS 2>/dev/null | grep "Total time:" | awk '{print $3}')
    instrumented_times+=($time_ms)
    echo "  Run $i: $time_ms ms"
done
echo ""

# Calculate averages using awk
baseline_avg=$(printf '%s\n' "${baseline_times[@]}" | awk '{sum+=$1} END {print sum/NR}')
instrumented_avg=$(printf '%s\n' "${instrumented_times[@]}" | awk '{sum+=$1} END {print sum/NR}')

# Calculate overhead
overhead=$(echo "scale=2; (($instrumented_avg - $baseline_avg) / $baseline_avg) * 100" | bc)

echo "=============================="
echo "Results:"
echo "  Baseline average:     $baseline_avg ms"
echo "  Instrumented average: $instrumented_avg ms"
echo "  Overhead:             $overhead%"
echo "=============================="
