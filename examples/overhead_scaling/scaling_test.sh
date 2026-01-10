#!/bin/bash
# Test overhead scaling across different workload sizes

echo "Overhead Scaling Across Workload Sizes"
echo "======================================="
echo ""

# Test configurations (size, iterations to keep ~same total ops)
declare -a configs=(
    "1000:10000"
    "5000:2000"
    "10000:1000"
    "50000:200"
    "100000:100"
)

RUNS=5

for config in "${configs[@]}"; do
    IFS=':' read -r size iters <<< "$config"

    echo "--------------------------------------"
    echo "Size: $size elements, Iterations: $iters"
    echo "Total operations: ~$((size * iters * 3))"
    echo "--------------------------------------"

    # Warmup
    for i in {1..5}; do
        ./benchmark_baseline $size $iters > /dev/null 2>&1
        ./benchmark_instrumented $size $iters > /dev/null 2>&1
    done

    # Measure baseline
    baseline_sum=0
    for i in $(seq 1 $RUNS); do
        time_ms=$(./benchmark_baseline $size $iters | grep "Total time:" | awk '{print $3}')
        baseline_sum=$(echo "$baseline_sum + $time_ms" | bc)
    done
    baseline_avg=$(echo "scale=3; $baseline_sum / $RUNS" | bc)

    # Measure instrumented
    inst_sum=0
    for i in $(seq 1 $RUNS); do
        time_ms=$(./benchmark_instrumented $size $iters 2>/dev/null | grep "Total time:" | awk '{print $3}')
        inst_sum=$(echo "$inst_sum + $time_ms" | bc)
    done
    inst_avg=$(echo "scale=3; $inst_sum / $RUNS" | bc)

    # Calculate overhead
    overhead=$(echo "scale=2; (($inst_avg - $baseline_avg) / $baseline_avg) * 100" | bc)

    echo "  Baseline:     $baseline_avg ms"
    echo "  Instrumented: $inst_avg ms"
    echo "  Overhead:     $overhead%"
    echo ""
done

echo "======================================="
echo "Conclusion: Check if overhead remains constant"
echo "across different workload sizes."
echo "======================================="
