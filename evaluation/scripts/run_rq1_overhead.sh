#!/bin/bash
# RQ1: Instrumentation Overhead Evaluation
# Compares OptiWeave instrumentation overhead against baselines

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OPTIWEAVE_BIN="${SCRIPT_DIR}/../../build/optiweave"
RESULTS_DIR="${SCRIPT_DIR}/../results/rq1"
POLYBENCH_DIR="${SCRIPT_DIR}/../benchmarks/polybench/PolyBenchC-4.2.1"

mkdir -p "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR/correctness"

# Configuration
ITERATIONS=10
WARMUP_ITERATIONS=3
KERNELS=(
    "linear-algebra/blas/gemm/gemm.c"
    "linear-algebra/blas/gemver/gemver.c"
    "linear-algebra/solvers/cholesky/cholesky.c"
    "stencils/jacobi-2d/jacobi-2d.c"
    "medley/nussinov/nussinov.c"
    "linear-algebra/kernels/2mm/2mm.c"
    "datamining/correlation/correlation.c"
    "stencils/heat-3d/heat-3d.c"
    "linear-algebra/kernels/atax/atax.c"
    "medley/floyd-warshall/floyd-warshall.c"
)

echo "RQ1: Overhead Analysis - Starting evaluation"
echo "Iterations per benchmark: $ITERATIONS"
echo "Output: $RESULTS_DIR"
echo ""

# CSV headers
echo "benchmark,mode,iteration,time_sec,overhead_pct" > "$RESULTS_DIR/overhead_results.csv"
echo "benchmark,baseline_median,optiweave_median,overhead_pct,baseline_stddev,optiweave_stddev,correctness" > "$RESULTS_DIR/summary_stats.csv"

for kernel in "${KERNELS[@]}"; do
    kernel_name=$(basename "$kernel" .c)
    kernel_path="$POLYBENCH_DIR/$kernel"

    echo "=== Evaluating: $kernel_name ==="

    if [ ! -f "$kernel_path" ]; then
        echo "  [SKIP] File not found: $kernel_path"
        continue
    fi

    # 1. Baseline (no instrumentation)
    echo "  [1/4] Baseline compilation..."
    gcc -O2 -I"$POLYBENCH_DIR/utilities" "$kernel_path" \
        "$POLYBENCH_DIR/utilities/polybench.c" \
        -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_baseline"

    # Warmup runs
    for i in $(seq 1 $WARMUP_ITERATIONS); do
        "/tmp/${kernel_name}_baseline" > /dev/null 2>&1
    done

    # Capture baseline output for correctness check
    "/tmp/${kernel_name}_baseline" > "$RESULTS_DIR/correctness/${kernel_name}_baseline.out" 2>&1 || true

    # Measured runs
    baseline_times=()
    for i in $(seq 1 $ITERATIONS); do
        time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_baseline" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
        baseline_times+=("$time_output")
        echo "$kernel_name,baseline,$i,$time_output,0" >> "$RESULTS_DIR/overhead_results.csv"
    done

    # Calculate baseline statistics (mean, median, stddev)
    baseline_median=$(echo "${baseline_times[@]}" | tr ' ' '\n' | sort -n | awk '{arr[NR]=$1} END {if(NR%2==1) print arr[(NR+1)/2]; else print (arr[NR/2]+arr[NR/2+1])/2}')
    baseline_mean=$(echo "${baseline_times[@]}" | tr ' ' '\n' | awk '{sum+=$1} END {print sum/NR}')
    baseline_stddev=$(echo "${baseline_times[@]}" | tr ' ' '\n' | awk -v mean=$baseline_mean '{sum+=($1-mean)^2} END {print sqrt(sum/NR)}')

    echo "  Baseline: median=${baseline_median}s, mean=${baseline_mean}s, stddev=${baseline_stddev}s"

    # Use median for baseline comparison (more robust to outliers)
    avg_baseline=$baseline_median

    # 2. OptiWeave (array subscripts)
    echo "  [2/4] OptiWeave array instrumentation..."
    mkdir -p "/tmp/ow_${kernel_name}_array"
    "$OPTIWEAVE_BIN" "$kernel_path" --array-subscripts \
        --output-dir="/tmp/ow_${kernel_name}_array" -- 2>/dev/null || true

    # Compile the transformed file
    transformed_file="/tmp/ow_${kernel_name}_array/$(basename $kernel_path)"
    kernel_dir=$(dirname "$kernel_path")
    correctness_status="UNTESTED"

    if [ -f "$transformed_file" ]; then
        g++ -O2 -I"${SCRIPT_DIR}/../../templates" \
            -I"$kernel_dir" \
            -I"$POLYBENCH_DIR/utilities" \
            "$transformed_file" \
            "$POLYBENCH_DIR/utilities/polybench.c" \
            -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_array" 2>/dev/null || true

        if [ -f "/tmp/${kernel_name}_array" ]; then
            # Warmup runs
            for i in $(seq 1 $WARMUP_ITERATIONS); do
                "/tmp/${kernel_name}_array" > /dev/null 2>&1 || true
            done

            # Correctness check
            "/tmp/${kernel_name}_array" > "$RESULTS_DIR/correctness/${kernel_name}_optiweave.out" 2>&1 || true

            if diff -q "$RESULTS_DIR/correctness/${kernel_name}_baseline.out" \
                      "$RESULTS_DIR/correctness/${kernel_name}_optiweave.out" > /dev/null 2>&1; then
                correctness_status="PASS"
                echo "  ✓ Correctness: PASS"
            else
                correctness_status="FAIL"
                echo "  ✗ Correctness: FAIL (outputs differ)"
            fi

            # Measured runs
            optiweave_times=()
            for i in $(seq 1 $ITERATIONS); do
                time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_array" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
                optiweave_times+=("$time_output")
                overhead=$(echo "scale=2; ($time_output - $avg_baseline) / $avg_baseline * 100" | bc)
                echo "$kernel_name,optiweave_array,$i,$time_output,$overhead" >> "$RESULTS_DIR/overhead_results.csv"
            done

            # Calculate OptiWeave statistics
            optiweave_median=$(echo "${optiweave_times[@]}" | tr ' ' '\n' | sort -n | awk '{arr[NR]=$1} END {if(NR%2==1) print arr[(NR+1)/2]; else print (arr[NR/2]+arr[NR/2+1])/2}')
            optiweave_mean=$(echo "${optiweave_times[@]}" | tr ' ' '\n' | awk '{sum+=$1} END {print sum/NR}')
            optiweave_stddev=$(echo "${optiweave_times[@]}" | tr ' ' '\n' | awk -v mean=$optiweave_mean '{sum+=($1-mean)^2} END {print sqrt(sum/NR)}')
            overhead_pct=$(echo "scale=2; ($optiweave_median - $avg_baseline) / $avg_baseline * 100" | bc)

            echo "  OptiWeave: median=${optiweave_median}s, stddev=${optiweave_stddev}s, overhead=${overhead_pct}%"

            # Save summary stats
            echo "$kernel_name,$baseline_median,$optiweave_median,$overhead_pct,$baseline_stddev,$optiweave_stddev,$correctness_status" >> "$RESULTS_DIR/summary_stats.csv"
        else
            echo "  ✗ Compilation failed"
            echo "$kernel_name,$baseline_median,FAILED,N/A,$baseline_stddev,N/A,COMPILATION_FAILED" >> "$RESULTS_DIR/summary_stats.csv"
        fi
    else
        echo "  ✗ Transformation failed"
        echo "$kernel_name,$baseline_median,FAILED,N/A,$baseline_stddev,N/A,TRANSFORMATION_FAILED" >> "$RESULTS_DIR/summary_stats.csv"
    fi

    # 3. OptiWeave (arithmetic ops)
    echo "  [3/4] OptiWeave arithmetic instrumentation..."
    mkdir -p "/tmp/ow_${kernel_name}_arith"
    "$OPTIWEAVE_BIN" "$kernel_path" --arithmetic-ops \
        --output-dir="/tmp/ow_${kernel_name}_arith" -- 2>/dev/null || true

    # Compile the transformed file
    transformed_file="/tmp/ow_${kernel_name}_arith/$(basename $kernel_path)"
    kernel_dir=$(dirname "$kernel_path")
    if [ -f "$transformed_file" ]; then
        g++ -O2 -I"${SCRIPT_DIR}/../../templates" \
            -I"$kernel_dir" \
            -I"$POLYBENCH_DIR/utilities" \
            "$transformed_file" \
            "$POLYBENCH_DIR/utilities/polybench.c" \
            -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_arith" 2>/dev/null || true

        if [ -f "/tmp/${kernel_name}_arith" ]; then
            for i in $(seq 1 $ITERATIONS); do
                time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_arith" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
                overhead=$(echo "scale=2; ($time_output - $avg_baseline) / $avg_baseline * 100" | bc)
                echo "$kernel_name,optiweave_arith,$i,$time_output,$overhead" >> "$RESULTS_DIR/overhead_results.csv"
            done
        fi
    fi

    # 4. gprof (baseline comparison)
    echo "  [4/4] gprof profiling..."
    gcc -O2 -pg -I"$POLYBENCH_DIR/utilities" "$kernel_path" \
        "$POLYBENCH_DIR/utilities/polybench.c" \
        -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_gprof"

    for i in $(seq 1 $ITERATIONS); do
        time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_gprof" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
        overhead=$(echo "scale=2; ($time_output - $avg_baseline) / $avg_baseline * 100" | bc)
        echo "$kernel_name,gprof,$i,$time_output,$overhead" >> "$RESULTS_DIR/overhead_results.csv"
    done

    echo "  ✓ Complete"
    echo ""
done

echo "======================================================================="
echo "RQ1 evaluation complete!"
echo "======================================================================="
echo ""
echo "Results saved to:"
echo "  - Raw data:        $RESULTS_DIR/overhead_results.csv"
echo "  - Summary stats:   $RESULTS_DIR/summary_stats.csv"
echo "  - Correctness:     $RESULTS_DIR/correctness/"
echo ""
echo "Next steps:"
echo "  1. Review summary: cat $RESULTS_DIR/summary_stats.csv"
echo "  2. Run analysis:   ./simple_analyze_rq1.sh"
echo "  3. Generate tables and graphs for thesis"
echo ""
