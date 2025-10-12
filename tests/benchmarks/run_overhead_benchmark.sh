#!/bin/bash
# Script to measure OptiWeave overhead by comparing baseline vs instrumented code

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║           OptiWeave Overhead Measurement Tool                ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

cd "$PROJECT_ROOT"

# Clean previous builds
echo "Cleaning previous builds..."
rm -f tests/benchmarks/benchmark_baseline
rm -f tests/benchmarks/benchmark_instrumented
rm -f tests/benchmarks/benchmark_overhead_transformed.cpp

# Save a copy of the original file (transformer modifies in-place)
cp tests/benchmarks/benchmark_overhead.cpp tests/benchmarks/benchmark_overhead_original.cpp

# Build baseline (no instrumentation)
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 1: Building BASELINE (no instrumentation)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
g++ -std=c++20 -O2 \
    tests/benchmarks/benchmark_overhead_original.cpp \
    -o tests/benchmarks/benchmark_baseline

echo "✓ Baseline built successfully"

# Transform the code
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 2: Transforming code for INSTRUMENTATION"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
./build/optiweave \
    tests/benchmarks/benchmark_overhead_original.cpp \
    --array-subscripts \
    --arithmetic-ops \
    -o tests/benchmarks/benchmark_overhead.cpp

echo "✓ Code transformed successfully"

# Build instrumented version with stats only (minimal overhead)
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 3: Building INSTRUMENTED version (stats only)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
g++ -std=c++20 -O2 \
    -DOPTIWEAVE_ENABLE_STATS \
    -I./templates -I./include \
    tests/benchmarks/benchmark_overhead.cpp \
    build/liboptiweave_runtime.a \
    -o tests/benchmarks/benchmark_instrumented

echo "✓ Instrumented version built successfully"

# Run baseline
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 4: Running BASELINE benchmark"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
./tests/benchmarks/benchmark_baseline > /tmp/optiweave_baseline.txt
cat /tmp/optiweave_baseline.txt

# Run instrumented
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 5: Running INSTRUMENTED benchmark"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
OPTIWEAVE_STATS=0 ./tests/benchmarks/benchmark_instrumented > /tmp/optiweave_instrumented.txt
cat /tmp/optiweave_instrumented.txt

# Create overhead report
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 6: Generating overhead report"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Extract overhead percentages from output
python3 - <<'EOF'
import re
import sys

# Read outputs
with open('/tmp/optiweave_baseline.txt', 'r') as f:
    baseline = f.read()

with open('/tmp/optiweave_instrumented.txt', 'r') as f:
    instrumented = f.read()

# Parse results
def extract_avg_time(text, benchmark_name):
    # Look for the instrumented line for each benchmark
    pattern = rf'{re.escape(benchmark_name)} \(instrumented\)\s+│\s+[\d.]+\s+│\s+[\d.]+\s+│\s+([\d.]+)'
    match = re.search(pattern, text)
    if match:
        return float(match.group(1))
    return None

benchmarks = [
    "Array",
    "Arithmetic",
    "Mixed",
    "Function"
]

print("╔══════════════════════════════════════════════════════════════╗")
print("║              Overhead Comparison Report                      ║")
print("╚══════════════════════════════════════════════════════════════╝")
print()
print("┌────────────────────┬─────────────┬─────────────┬────────────┐")
print("│ Benchmark          │ Baseline    │ Instrumented│ Overhead   │")
print("├────────────────────┼─────────────┼─────────────┼────────────┤")

total_overhead = 0
count = 0

for bench in benchmarks:
    baseline_time = extract_avg_time(baseline, bench)
    instrumented_time = extract_avg_time(instrumented, bench)

    if baseline_time and instrumented_time:
        overhead = ((instrumented_time - baseline_time) / baseline_time) * 100
        total_overhead += overhead
        count += 1

        print(f"│ {bench:18s} │ {baseline_time:9.3f}ms │ {instrumented_time:9.3f}ms │ {overhead:9.2f}% │")

print("└────────────────────┴─────────────┴─────────────┴────────────┘")
print()

if count > 0:
    avg_overhead = total_overhead / count
    print(f"Average overhead: {avg_overhead:.2f}%")
    print()

    if avg_overhead < 1.0:
        print("✓ EXCELLENT - Target achieved! (<1% overhead)")
    elif avg_overhead < 5.0:
        print("✓ GOOD - Within acceptable range (<5% overhead)")
    elif avg_overhead < 10.0:
        print("⚠ ACCEPTABLE - Could be improved (<10% overhead)")
    else:
        print("✗ HIGH - Optimization needed (>10% overhead)")
else:
    print("⚠ Could not parse results")
    sys.exit(1)
EOF

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Overhead measurement complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Files saved:"
echo "  - Baseline results: /tmp/optiweave_baseline.txt"
echo "  - Instrumented results: /tmp/optiweave_instrumented.txt"
echo ""
