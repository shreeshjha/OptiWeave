#!/bin/bash
# Collect comprehensive scaling results

echo "OptiWeave Overhead Scaling Results"
echo "======================================"
echo ""

# Test configurations
declare -a tests=(
    "100:100000:Small"
    "1000:10000:Medium"
    "10000:1000:Large"
    "100000:100:Very Large"
)

for test in "${tests[@]}"; do
    IFS=':' read -r size iters label <<< "$test"

    echo "--------------------------------------"
    echo "Test: $label workload"
    echo "Size: $size elements, Iterations: $iters"
    echo "--------------------------------------"

    echo ""
    echo "BASELINE:"
    ./benchmark_baseline $size $iters
    echo ""

    echo "INSTRUMENTED:"
    ./benchmark_instrumented $size $iters 2>/dev/null
    echo ""
    echo ""
done
