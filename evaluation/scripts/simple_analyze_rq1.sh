#!/bin/bash
# Simple RQ1 analysis without pandas

CSV="/Users/shreesh/Dev/Github/OptiWeave/evaluation/results/rq1/overhead_results.csv"

echo "======================================================================"
echo "RQ1: Instrumentation Overhead Analysis"
echo "======================================================================"
echo ""

echo "Mean Overhead by Mode:"
echo "----------------------"

# Calculate mean overhead for each mode
for mode in baseline gprof optiweave_array optiweave_arith; do
    avg=$(grep ",$mode," "$CSV" | awk -F',' '{sum+=$5; count++} END {if(count>0) print sum/count; else print "N/A"}')
    count=$(grep -c ",$mode," "$CSV")

    if [ "$count" -gt 0 ]; then
        printf "%-20s: %8s%% (n=%d)\n" "$mode" "$avg" "$count"
    fi
done

echo ""
echo "======================================================================"
echo "Per-Benchmark Overhead (OptiWeave Array vs Baseline):"
echo "======================================================================"
echo ""

# Get unique benchmarks
benchmarks=$(cut -d',' -f1 "$CSV" | sort -u | grep -v "benchmark")

printf "%-20s %12s %12s %12s\n" "Benchmark" "Baseline(s)" "Array(s)" "Overhead%"
echo "----------------------------------------------------------------------"

for bench in $benchmarks; do
    baseline_avg=$(grep "^$bench,baseline," "$CSV" | awk -F',' '{sum+=$4; count++} END {print sum/count}')
    array_avg=$(grep "^$bench,optiweave_array," "$CSV" | awk -F',' '{sum+=$4; count++} END {if(count>0) print sum/count; else print "N/A"}')

    if [ "$array_avg" != "N/A" ] && [ -n "$array_avg" ]; then
        overhead=$(echo "scale=2; ($array_avg - $baseline_avg) / $baseline_avg * 100" | bc)
        printf "%-20s %12.3f %12.3f %12.2f%%\n" "$bench" "$baseline_avg" "$array_avg" "$overhead"
    else
        printf "%-20s %12.3f %12s %12s\n" "$bench" "$baseline_avg" "FAILED" "N/A"
    fi
done

echo ""
echo "======================================================================"
echo "Summary Statistics:"
echo "======================================================================"

# Count successful vs failed
total_benchmarks=$(echo "$benchmarks" | wc -l | tr -d ' ')
successful=$(grep "optiweave_array" "$CSV" | cut -d',' -f1 | sort -u | wc -l | tr -d ' ')

echo "Total benchmarks: $total_benchmarks"
echo "Successful OptiWeave array: $successful"
echo "Failed: $((total_benchmarks - successful))"
echo ""

echo "Results saved to: $CSV"
echo "======================================================================"
