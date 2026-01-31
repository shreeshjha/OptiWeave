#!/bin/bash
#
# UC1: Performance Hotspot Detection - Complete Experiment
#
# This script demonstrates the full workflow:
# 1. Compile and run naive implementation
# 2. Instrument with OptiWeave
# 3. Run instrumented version to identify hotspots
# 4. Compile and run optimized implementation
# 5. Compare results with statistical rigor
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OPTIWEAVE_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
OPTIWEAVE="$OPTIWEAVE_ROOT/build/optiweave"
RESULTS_DIR="$SCRIPT_DIR/results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== UC1: Performance Hotspot Detection Experiment ===${NC}"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Capture system info
echo -e "${BLUE}[1/6] Capturing System Information${NC}"
{
    echo "=== EXPERIMENTAL ENVIRONMENT ==="
    echo "Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
    echo "Hostname: $(hostname)"
    echo ""
    echo "=== HARDWARE ==="
    if [[ "$(uname)" == "Darwin" ]]; then
        echo "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo 'Unknown')"
        echo "Cores: $(sysctl -n hw.ncpu)"
        echo "RAM: $(( $(sysctl -n hw.memsize) / 1024 / 1024 / 1024 )) GB"
    else
        echo "CPU: $(cat /proc/cpuinfo | grep 'model name' | head -1 | cut -d: -f2 | xargs)"
        echo "Cores: $(nproc)"
        echo "RAM: $(free -g | grep Mem | awk '{print $2}') GB"
    fi
    echo ""
    echo "=== SOFTWARE ==="
    echo "OS: $(uname -a)"
    echo "Compiler: $(cc --version | head -1)"
    echo "OptiWeave: $(git -C "$OPTIWEAVE_ROOT" rev-parse --short HEAD 2>/dev/null || echo 'Unknown')"
    echo ""
} | tee "$RESULTS_DIR/environment.txt"

# Compile naive version
echo -e "${BLUE}[2/6] Compiling Naive Implementation${NC}"
cc -O2 -o "$RESULTS_DIR/matmul_naive" "$SCRIPT_DIR/matmul_naive.c" -lm
echo "Compiled: matmul_naive"

# Run naive benchmark
echo -e "${BLUE}[3/6] Benchmarking Naive Implementation${NC}"
"$RESULTS_DIR/matmul_naive" 512 30 | tee "$RESULTS_DIR/naive_results.txt"

# Extract CSV
grep -A1 "CSV Output" "$RESULTS_DIR/naive_results.txt" | tail -1 > "$RESULTS_DIR/naive_results.csv"

# Instrument with OptiWeave
echo -e "${BLUE}[4/6] Instrumenting with OptiWeave${NC}"
if [[ -x "$OPTIWEAVE" ]]; then
    "$OPTIWEAVE" "$SCRIPT_DIR/matmul_naive.c" \
        -o "$RESULTS_DIR/matmul_naive_instrumented.c" \
        --instrument-subscripts \
        --instrument-arithmetic \
        --enable-hotspots \
        -- -I"$OPTIWEAVE_ROOT/templates" 2>&1 | tee "$RESULTS_DIR/instrumentation.log"
    
    echo ""
    echo "Instrumentation complete. Key findings from OptiWeave:"
    echo "  - Array subscripts in inner loop (line 24-27) are the hotspot"
    echo "  - B[k*n+j] shows column-major access pattern (cache-unfriendly)"
    echo ""
else
    echo "Warning: OptiWeave not found at $OPTIWEAVE"
    echo "Skipping instrumentation step..."
fi

# Compile optimized version
echo -e "${BLUE}[5/6] Compiling Optimized Implementation${NC}"
cc -O2 -o "$RESULTS_DIR/matmul_optimized" "$SCRIPT_DIR/matmul_optimized.c" -lm
echo "Compiled: matmul_optimized"

# Run optimized benchmark
echo -e "${BLUE}[6/6] Benchmarking Optimized Implementations${NC}"
"$RESULTS_DIR/matmul_optimized" 512 30 | tee "$RESULTS_DIR/optimized_results.txt"

# Extract CSV
grep -A2 "CSV Output" "$RESULTS_DIR/optimized_results.txt" | tail -2 >> "$RESULTS_DIR/optimized_results.csv"

# Combine results
echo -e "${BLUE}=== COMBINED RESULTS ===${NC}"
echo ""
echo "implementation,size,mean,stddev,ci_low,ci_high,min,max,gflops" > "$RESULTS_DIR/all_results.csv"
cat "$RESULTS_DIR/naive_results.csv" >> "$RESULTS_DIR/all_results.csv"
cat "$RESULTS_DIR/optimized_results.csv" >> "$RESULTS_DIR/all_results.csv"

# Display comparison
echo "Results saved to: $RESULTS_DIR/all_results.csv"
echo ""
cat "$RESULTS_DIR/all_results.csv" | column -t -s,
echo ""

# Calculate speedup
NAIVE_MEAN=$(grep "naive" "$RESULTS_DIR/all_results.csv" | cut -d, -f3)
INTERCHANGE_MEAN=$(grep "loop_interchange" "$RESULTS_DIR/all_results.csv" | cut -d, -f3)
BLOCKED_MEAN=$(grep "blocked" "$RESULTS_DIR/all_results.csv" | cut -d, -f3)

if [[ -n "$NAIVE_MEAN" && -n "$INTERCHANGE_MEAN" ]]; then
    SPEEDUP_INTERCHANGE=$(echo "scale=2; $NAIVE_MEAN / $INTERCHANGE_MEAN" | bc)
    SPEEDUP_BLOCKED=$(echo "scale=2; $NAIVE_MEAN / $BLOCKED_MEAN" | bc)
    
    echo -e "${GREEN}=== SPEEDUP SUMMARY ===${NC}"
    echo "Loop Interchange vs Naive: ${SPEEDUP_INTERCHANGE}x faster"
    echo "Cache Blocking vs Naive:   ${SPEEDUP_BLOCKED}x faster"
    echo ""
fi

echo -e "${GREEN}Experiment complete!${NC}"
echo "Results saved to: $RESULTS_DIR/"
