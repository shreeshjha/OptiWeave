#!/usr/bin/env python3
"""
RQ1 Analysis Script - Overhead Analysis
Processes overhead results and generates tables/graphs for thesis
"""

import pandas as pd
import numpy as np
import sys
from pathlib import Path

def analyze_overhead(csv_path):
    """Analyze overhead results and generate statistics"""

    print("=" * 70)
    print("RQ1: Instrumentation Overhead Analysis")
    print("=" * 70)
    print()

    # Read results
    df = pd.read_csv(csv_path)

    # Calculate statistics by benchmark and mode
    stats = df.groupby(['benchmark', 'mode']).agg({
        'time_sec': ['mean', 'std', 'min', 'max'],
        'overhead_pct': ['mean', 'std']
    }).round(4)

    print("Summary Statistics by Benchmark:")
    print(stats)
    print()

    # Overall statistics by mode
    mode_stats = df.groupby('mode').agg({
        'overhead_pct': ['mean', 'std', 'min', 'max', 'median']
    }).round(2)

    print("=" * 70)
    print("Overall Overhead by Instrumentation Mode:")
    print("=" * 70)
    for mode in mode_stats.index:
        if mode == 'baseline':
            continue
        stats = mode_stats.loc[mode, 'overhead_pct']
        print(f"\n{mode}:")
        print(f"  Mean overhead:   {stats['mean']:>8.2f}%")
        print(f"  Std deviation:   {stats['std']:>8.2f}%")
        print(f"  Min overhead:    {stats['min']:>8.2f}%")
        print(f"  Max overhead:    {stats['max']:>8.2f}%")
        print(f"  Median overhead: {stats['median']:>8.2f}%")

    print()
    print("=" * 70)
    print("Thesis Table: Mean Overhead Comparison")
    print("=" * 70)

    # Create thesis-ready table
    pivot = df.groupby(['benchmark', 'mode'])['overhead_pct'].mean().unstack(fill_value=0)
    pivot = pivot.round(2)

    # Reorder columns
    col_order = ['baseline', 'gprof', 'optiweave_array', 'optiweave_arith']
    pivot = pivot[[col for col in col_order if col in pivot.columns]]

    print(pivot.to_string())
    print()

    # Save processed results
    output_dir = Path(csv_path).parent
    pivot.to_csv(output_dir / 'overhead_summary.csv')
    stats.to_csv(output_dir / 'overhead_detailed_stats.csv')

    print(f"✓ Saved summary to: {output_dir / 'overhead_summary.csv'}")
    print(f"✓ Saved detailed stats to: {output_dir / 'overhead_detailed_stats.csv'}")
    print()

    # Key findings for thesis
    print("=" * 70)
    print("KEY FINDINGS for Thesis:")
    print("=" * 70)
    optiweave_array_mean = mode_stats.loc['optiweave_array', ('overhead_pct', 'mean')] if 'optiweave_array' in mode_stats.index else 0
    optiweave_arith_mean = mode_stats.loc['optiweave_arith', ('overhead_pct', 'mean')] if 'optiweave_arith' in mode_stats.index else 0
    gprof_mean = mode_stats.loc['gprof', ('overhead_pct', 'mean')] if 'gprof' in mode_stats.index else 0

    print(f"1. OptiWeave array instrumentation adds {optiweave_array_mean:.1f}% overhead on average")
    print(f"2. OptiWeave arithmetic instrumentation adds {optiweave_arith_mean:.1f}% overhead on average")
    print(f"3. gprof adds {gprof_mean:.1f}% overhead on average")
    print()
    print("These results answer RQ1 regarding the trade-off between")
    print("instrumentation granularity and runtime overhead.")
    print("=" * 70)

if __name__ == '__main__':
    results_path = Path(__file__).parent.parent / 'results' / 'rq1' / 'overhead_results.csv'

    if not results_path.exists():
        print(f"Error: Results file not found: {results_path}")
        print("Run evaluation/scripts/run_rq1_overhead.sh first")
        sys.exit(1)

    analyze_overhead(results_path)
