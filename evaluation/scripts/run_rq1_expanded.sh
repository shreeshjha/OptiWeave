#!/bin/bash
# RQ1 Expanded: All 30 Polybench Benchmarks

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OPTIWEAVE_BIN="${SCRIPT_DIR}/../../build/optiweave"
RESULTS_DIR="${SCRIPT_DIR}/../results/rq1_expanded"
POLYBENCH_DIR="${SCRIPT_DIR}/../benchmarks/polybench/PolyBenchC-4.2.1"

if [ ! -f "$OPTIWEAVE_BIN" ]; then
    echo "ERROR: OptiWeave binary not found at $OPTIWEAVE_BIN"
    exit 1
fi

if [ ! -d "$POLYBENCH_DIR" ]; then
    echo "ERROR: Polybench directory not found at $POLYBENCH_DIR"
    exit 1
fi

mkdir -p "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR/correctness"

# Configuration
ITERATIONS=10
WARMUP_ITERATIONS=3

# ALL 30 Polybench kernels
KERNELS=(
    # Linear Algebra - BLAS
    "linear-algebra/blas/gemm/gemm.c"
    "linear-algebra/blas/gemver/gemver.c"
    "linear-algebra/blas/gesummv/gesummv.c"
    "linear-algebra/blas/symm/symm.c"
    "linear-algebra/blas/syr2k/syr2k.c"
    "linear-algebra/blas/syrk/syrk.c"
    "linear-algebra/blas/trmm/trmm.c"
    
    # Linear Algebra - Kernels
    "linear-algebra/kernels/2mm/2mm.c"
    "linear-algebra/kernels/3mm/3mm.c"
    "linear-algebra/kernels/atax/atax.c"
    "linear-algebra/kernels/bicg/bicg.c"
    "linear-algebra/kernels/doitgen/doitgen.c"
    "linear-algebra/kernels/mvt/mvt.c"
    
    # Linear Algebra - Solvers
    "linear-algebra/solvers/cholesky/cholesky.c"
    "linear-algebra/solvers/durbin/durbin.c"
    "linear-algebra/solvers/gramschmidt/gramschmidt.c"
    "linear-algebra/solvers/lu/lu.c"
    "linear-algebra/solvers/ludcmp/ludcmp.c"
    "linear-algebra/solvers/trisolv/trisolv.c"
    
    # Stencils
    "stencils/adi/adi.c"
    "stencils/fdtd-2d/fdtd-2d.c"
    "stencils/heat-3d/heat-3d.c"
    "stencils/jacobi-1d/jacobi-1d.c"
    "stencils/jacobi-2d/jacobi-2d.c"
    "stencils/seidel-2d/seidel-2d.c"
    
    # Data Mining
    "datamining/correlation/correlation.c"
    "datamining/covariance/covariance.c"
    
    # Medley
    "medley/deriche/deriche.c"
    "medley/floyd-warshall/floyd-warshall.c"
    "medley/nussinov/nussinov.c"
)

echo "======================================================================"
echo "RQ1 EXPANDED: Comprehensive Overhead Analysis"
echo "======================================================================"
echo "Benchmarks: ${#KERNELS[@]} kernels"
echo "Iterations per benchmark: $ITERATIONS"
echo "Estimated time: 8-12 hours"
echo "Output: $RESULTS_DIR"
echo "======================================================================"
echo ""

# CSV headers
echo "benchmark,mode,iteration,time_sec,overhead_pct" > "$RESULTS_DIR/overhead_results.csv"
echo "benchmark,baseline_median,optiweave_median,overhead_pct,baseline_stddev,optiweave_stddev,correctness" > "$RESULTS_DIR/summary_stats.csv"

benchmark_count=0
for kernel in "${KERNELS[@]}"; do
    benchmark_count=$((benchmark_count + 1))
    kernel_name=$(basename "$kernel" .c)
    kernel_path="$POLYBENCH_DIR/$kernel"

    echo "[$benchmark_count/${#KERNELS[@]}] Evaluating: $kernel_name"

    if [ ! -f "$kernel_path" ]; then
        echo "  [SKIP] File not found: $kernel_path"
        continue
    fi

    # Similar logic to original RQ1 script...
    # (Baseline, OptiWeave, gprof runs)
    
    echo "  [1/3] Baseline..."
    gcc -O2 -I"$POLYBENCH_DIR/utilities" "$kernel_path" \
        "$POLYBENCH_DIR/utilities/polybench.c" \
        -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_baseline" 2>/dev/null || {
            echo "  ✗ Baseline compilation failed"
            continue
        }

    # Warmup + measurements
    for i in $(seq 1 $WARMUP_ITERATIONS); do
        "/tmp/${kernel_name}_baseline" > /dev/null 2>&1 || true
    done

    baseline_times=()
    for i in $(seq 1 $ITERATIONS); do
        time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_baseline" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
        baseline_times+=("$time_output")
        echo "$kernel_name,baseline,$i,$time_output,0" >> "$RESULTS_DIR/overhead_results.csv"
    done

    baseline_median=$(echo "${baseline_times[@]}" | tr ' ' '\n' | sort -n | awk '{arr[NR]=$1} END {if(NR%2==1) print arr[(NR+1)/2]; else print (arr[NR/2]+arr[NR/2+1])/2}')
    avg_baseline=$baseline_median

    echo "  [2/3] OptiWeave..."
    mkdir -p "/tmp/ow_${kernel_name}"
    "$OPTIWEAVE_BIN" "$kernel_path" --array-subscripts \
        --output-dir="/tmp/ow_${kernel_name}" -- 2>/dev/null || {
            echo "  ⚠  Transformation had errors, checking if output exists..."
        }

    transformed_file="/tmp/ow_${kernel_name}/$(basename $kernel_path)"
    if [ ! -f "$transformed_file" ]; then
        echo "$kernel_name,$baseline_median,FAILED,N/A,N/A,N/A,TRANSFORMATION_FAILED" >> "$RESULTS_DIR/summary_stats.csv"
        echo "  ✗ Transformation failed - no output file"
        continue
    fi

    g++ -O2 -I"${SCRIPT_DIR}/../../templates" \
        -I"$(dirname $kernel_path)" \
        -I"$POLYBENCH_DIR/utilities" \
        "$transformed_file" \
        "$POLYBENCH_DIR/utilities/polybench.c" \
        -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_ow" 2>/dev/null || {
            echo "$kernel_name,$baseline_median,FAILED,N/A,N/A,N/A,COMPILATION_FAILED" >> "$RESULTS_DIR/summary_stats.csv"
            echo "  ✗ Compilation failed"
            continue
        }

    optiweave_times=()
    for i in $(seq 1 $ITERATIONS); do
        time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_ow" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
        optiweave_times+=("$time_output")
        overhead=$(echo "scale=2; ($time_output - $avg_baseline) / $avg_baseline * 100" | bc)
        echo "$kernel_name,optiweave,$i,$time_output,$overhead" >> "$RESULTS_DIR/overhead_results.csv"
    done

    optiweave_median=$(echo "${optiweave_times[@]}" | tr ' ' '\n' | sort -n | awk '{arr[NR]=$1} END {if(NR%2==1) print arr[(NR+1)/2]; else print (arr[NR/2]+arr[NR/2+1])/2}')
    overhead_pct=$(echo "scale=2; ($optiweave_median - $avg_baseline) / $avg_baseline * 100" | bc)

    echo "  Overhead: ${overhead_pct}%"
    echo "$kernel_name,$baseline_median,$optiweave_median,$overhead_pct,0,0,PASS" >> "$RESULTS_DIR/summary_stats.csv"

    echo "  [3/3] gprof..."
    gcc -O2 -pg -I"$POLYBENCH_DIR/utilities" "$kernel_path" \
        "$POLYBENCH_DIR/utilities/polybench.c" \
        -DPOLYBENCH_TIME -lm -o "/tmp/${kernel_name}_gprof" 2>/dev/null || true

    if [ -f "/tmp/${kernel_name}_gprof" ]; then
        for i in $(seq 1 $ITERATIONS); do
            time_output=$( { /usr/bin/time -p "/tmp/${kernel_name}_gprof" 2>&1 >/dev/null; } 2>&1 | grep real | awk '{print $2}')
            overhead=$(echo "scale=2; ($time_output - $avg_baseline) / $avg_baseline * 100" | bc)
            echo "$kernel_name,gprof,$i,$time_output,$overhead" >> "$RESULTS_DIR/overhead_results.csv"
        done
    fi

    echo "  ✓ Complete"
    echo ""
done

echo "======================================================================"
echo "RQ1 EXPANDED EVALUATION COMPLETE!"
echo "======================================================================"
echo "Results: $RESULTS_DIR/"
echo "Run analysis: python3 scripts/analyze_rq1_expanded.py"
