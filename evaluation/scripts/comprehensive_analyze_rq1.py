#!/usr/bin/env python3
"""
RQ1 Comprehensive Analysis Script
Processes overhead results with statistical tests and generates thesis-ready outputs
"""

import pandas as pd
import numpy as np
import json
import sys
from pathlib import Path
from scipy import stats

def remove_outliers_iqr(data, threshold=1.5):
    """Remove outliers using IQR method"""
    Q1 = data.quantile(0.25)
    Q3 = data.quantile(0.75)
    IQR = Q3 - Q1
    lower_bound = Q1 - threshold * IQR
    upper_bound = Q3 + threshold * IQR
    return data[(data >= lower_bound) & (data <= upper_bound)]

def calculate_confidence_interval(data, confidence=0.95):
    """Calculate confidence interval for mean"""
    n = len(data)
    if n < 2:
        return (data.mean(), data.mean())

    mean = data.mean()
    std_err = stats.sem(data)
    ci = std_err * stats.t.ppf((1 + confidence) / 2., n - 1)
    return (mean - ci, mean + ci)

def analyze_overhead(csv_path, output_dir):
    """Comprehensive overhead analysis with statistical tests"""

    print("=" * 80)
    print("RQ1: COMPREHENSIVE INSTRUMENTATION OVERHEAD ANALYSIS")
    print("=" * 80)
    print()

    # Read results
    df = pd.read_csv(csv_path)

    # Remove first iteration (warmup artifacts in data)
    df_clean = df[df['iteration'] > 1].copy()

    print(f"Loaded {len(df)} measurements")
    print(f"After removing first iteration: {len(df_clean)} measurements")
    print(f"Benchmarks: {df_clean['benchmark'].nunique()}")
    print(f"Modes: {df_clean['mode'].unique()}")
    print()

    # ========================================================================
    # 1. BASIC STATISTICS BY BENCHMARK
    # ========================================================================
    print("=" * 80)
    print("1. SUMMARY STATISTICS BY BENCHMARK")
    print("=" * 80)
    print()

    summary_stats = []
    benchmarks = df_clean['benchmark'].unique()

    for benchmark in benchmarks:
        bench_data = df_clean[df_clean['benchmark'] == benchmark]

        # Baseline stats
        baseline = bench_data[bench_data['mode'] == 'baseline']['time_sec']
        baseline_mean = baseline.mean()
        baseline_std = baseline.std()
        baseline_ci = calculate_confidence_interval(baseline)

        # OptiWeave stats
        optiweave = bench_data[bench_data['mode'] == 'optiweave']['time_sec']
        if len(optiweave) > 0:
            optiweave_mean = optiweave.mean()
            optiweave_std = optiweave.std()
            optiweave_ci = calculate_confidence_interval(optiweave)
            overhead = ((optiweave_mean - baseline_mean) / baseline_mean) * 100

            # Statistical significance test (paired t-test)
            if len(baseline) == len(optiweave):
                t_stat, p_value = stats.ttest_rel(optiweave, baseline)
            else:
                t_stat, p_value = stats.ttest_ind(optiweave, baseline)

            significant = "Yes" if p_value < 0.05 else "No"
        else:
            optiweave_mean = np.nan
            optiweave_std = np.nan
            optiweave_ci = (np.nan, np.nan)
            overhead = np.nan
            t_stat = np.nan
            p_value = np.nan
            significant = "N/A"

        # gprof stats
        gprof = bench_data[bench_data['mode'] == 'gprof']['time_sec']
        if len(gprof) > 0:
            gprof_mean = gprof.mean()
            gprof_overhead = ((gprof_mean - baseline_mean) / baseline_mean) * 100
        else:
            gprof_mean = np.nan
            gprof_overhead = np.nan

        summary_stats.append({
            'benchmark': benchmark,
            'baseline_mean': baseline_mean,
            'baseline_std': baseline_std,
            'baseline_ci_low': baseline_ci[0],
            'baseline_ci_high': baseline_ci[1],
            'optiweave_mean': optiweave_mean,
            'optiweave_std': optiweave_std,
            'optiweave_ci_low': optiweave_ci[0],
            'optiweave_ci_high': optiweave_ci[1],
            'optiweave_overhead_pct': overhead,
            'gprof_mean': gprof_mean,
            'gprof_overhead_pct': gprof_overhead,
            't_statistic': t_stat,
            'p_value': p_value,
            'significant': significant
        })

    summary_df = pd.DataFrame(summary_stats)
    summary_df = summary_df.sort_values('optiweave_overhead_pct')

    # Display summary
    print(summary_df[['benchmark', 'baseline_mean', 'optiweave_mean',
                      'optiweave_overhead_pct', 'p_value', 'significant']].to_string(index=False))
    print()

    # ========================================================================
    # 2. OVERALL STATISTICS
    # ========================================================================
    print("=" * 80)
    print("2. OVERALL STATISTICS")
    print("=" * 80)
    print()

    # Calculate overall metrics (excluding NaN)
    valid_overheads = summary_df['optiweave_overhead_pct'].dropna()
    valid_gprof = summary_df['gprof_overhead_pct'].dropna()

    print("OptiWeave Array Instrumentation:")
    print(f"  Mean overhead:       {valid_overheads.mean():>8.2f}%")
    print(f"  Median overhead:     {valid_overheads.median():>8.2f}%")
    print(f"  Std deviation:       {valid_overheads.std():>8.2f}%")
    print(f"  Min overhead:        {valid_overheads.min():>8.2f}%")
    print(f"  Max overhead:        {valid_overheads.max():>8.2f}%")
    print(f"  Quartiles:")
    print(f"    25th percentile:   {valid_overheads.quantile(0.25):>8.2f}%")
    print(f"    75th percentile:   {valid_overheads.quantile(0.75):>8.2f}%")
    print()

    print("gprof Profiling:")
    print(f"  Mean overhead:       {valid_gprof.mean():>8.2f}%")
    print(f"  Median overhead:     {valid_gprof.median():>8.2f}%")
    print(f"  Std deviation:       {valid_gprof.std():>8.2f}%")
    print(f"  Min overhead:        {valid_gprof.min():>8.2f}%")
    print(f"  Max overhead:        {valid_gprof.max():>8.2f}%")
    print()

    # Comparison test
    t_stat, p_value = stats.ttest_ind(valid_overheads, valid_gprof)
    print(f"Statistical Comparison (OptiWeave vs gprof):")
    print(f"  t-statistic:         {t_stat:>8.3f}")
    print(f"  p-value:             {p_value:>8.5f}")
    print(f"  Significant (α=0.05): {'Yes' if p_value < 0.05 else 'No'}")
    print()

    # ========================================================================
    # 3. OVERHEAD CATEGORIES
    # ========================================================================
    print("=" * 80)
    print("3. OVERHEAD CATEGORIZATION")
    print("=" * 80)
    print()

    def categorize_overhead(overhead):
        if pd.isna(overhead):
            return "Failed"
        elif overhead < 0:
            return "Negative (Speedup)"
        elif overhead < 5:
            return "Excellent (<5%)"
        elif overhead < 10:
            return "Good (5-10%)"
        elif overhead < 20:
            return "Acceptable (10-20%)"
        else:
            return "High (>20%)"

    summary_df['category'] = summary_df['optiweave_overhead_pct'].apply(categorize_overhead)

    category_counts = summary_df['category'].value_counts()
    print("Overhead Distribution:")
    for category, count in category_counts.items():
        pct = (count / len(summary_df)) * 100
        print(f"  {category:25s}: {count:>3d} benchmarks ({pct:>5.1f}%)")
    print()

    # ========================================================================
    # 4. STATISTICAL SIGNIFICANCE SUMMARY
    # ========================================================================
    print("=" * 80)
    print("4. STATISTICAL SIGNIFICANCE")
    print("=" * 80)
    print()

    significant_count = (summary_df['significant'] == 'Yes').sum()
    not_significant_count = (summary_df['significant'] == 'No').sum()

    print(f"Statistically significant differences (α=0.05):")
    print(f"  Significant:     {significant_count:>3d} benchmarks")
    print(f"  Not significant: {not_significant_count:>3d} benchmarks")
    print()

    print("Benchmarks with significant overhead:")
    sig_benchmarks = summary_df[summary_df['significant'] == 'Yes'][
        ['benchmark', 'optiweave_overhead_pct', 'p_value']
    ].sort_values('optiweave_overhead_pct', ascending=False)
    print(sig_benchmarks.to_string(index=False))
    print()

    # ========================================================================
    # 5. SAVE OUTPUTS
    # ========================================================================
    print("=" * 80)
    print("5. SAVING OUTPUTS")
    print("=" * 80)
    print()

    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    # Save comprehensive summary
    summary_csv = output_path / 'comprehensive_summary.csv'
    summary_df.to_csv(summary_csv, index=False)
    print(f"✓ Saved comprehensive summary: {summary_csv}")

    # Save thesis-ready table (LaTeX)
    latex_file = output_path / 'thesis_table_comprehensive.tex'
    with open(latex_file, 'w') as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\caption{OptiWeave Instrumentation Overhead on Polybench/C Benchmarks}\n")
        f.write("\\label{tab:rq1-overhead-full}\n")
        f.write("\\begin{tabular}{lrrrrl}\n")
        f.write("\\toprule\n")
        f.write("\\textbf{Benchmark} & \\textbf{Baseline (s)} & \\textbf{OptiWeave (s)} & ")
        f.write("\\textbf{Overhead (\\%)} & \\textbf{gprof (\\%)} & \\textbf{Sig.} \\\\\n")
        f.write("\\midrule\n")

        for _, row in summary_df.iterrows():
            sig_marker = "*" if row['significant'] == 'Yes' else ""
            f.write(f"{row['benchmark']:20s} & {row['baseline_mean']:>6.2f} & ")
            if pd.notna(row['optiweave_mean']):
                f.write(f"{row['optiweave_mean']:>6.2f} & {row['optiweave_overhead_pct']:>6.2f}")
            else:
                f.write("   N/A &    N/A")
            f.write(f" & {row['gprof_overhead_pct']:>6.2f} & {sig_marker:1s} \\\\\n")

        f.write("\\midrule\n")
        f.write("\\multicolumn{6}{l}{\\textbf{Summary Statistics:}} \\\\\n")
        f.write(f"\\multicolumn{{6}}{{l}}{{Mean OptiWeave overhead: {valid_overheads.mean():.2f}\\%}} \\\\\n")
        f.write(f"\\multicolumn{{6}}{{l}}{{Median OptiWeave overhead: {valid_overheads.median():.2f}\\%}} \\\\\n")
        f.write(f"\\multicolumn{{6}}{{l}}{{Mean gprof overhead: {valid_gprof.mean():.2f}\\%}} \\\\\n")
        f.write(f"\\multicolumn{{6}}{{l}}{{* Statistically significant at $\\alpha=0.05$}} \\\\\n")
        f.write("\\bottomrule\n")
        f.write("\\end{tabular}\n")
        f.write("\\end{table}\n")

    print(f"✓ Saved LaTeX table: {latex_file}")

    # Save markdown table
    markdown_file = output_path / 'thesis_table_comprehensive.md'
    with open(markdown_file, 'w') as f:
        f.write("# RQ1: Instrumentation Overhead Results\n\n")
        f.write("## Complete Benchmark Results\n\n")
        f.write("| Benchmark | Baseline (s) | OptiWeave (s) | Overhead (%) | gprof (%) | Significant |\n")
        f.write("|-----------|--------------|---------------|--------------|-----------|-------------|\n")

        for _, row in summary_df.iterrows():
            sig_marker = "✓" if row['significant'] == 'Yes' else ""
            if pd.notna(row['optiweave_mean']):
                f.write(f"| {row['benchmark']:20s} | {row['baseline_mean']:6.2f} | ")
                f.write(f"{row['optiweave_mean']:6.2f} | {row['optiweave_overhead_pct']:6.2f} | ")
                f.write(f"{row['gprof_overhead_pct']:6.2f} | {sig_marker:1s} |\n")

        f.write("\n## Summary Statistics\n\n")
        f.write(f"- **Mean OptiWeave overhead:** {valid_overheads.mean():.2f}%\n")
        f.write(f"- **Median OptiWeave overhead:** {valid_overheads.median():.2f}%\n")
        f.write(f"- **Mean gprof overhead:** {valid_gprof.mean():.2f}%\n")
        f.write(f"- **Benchmarks with <5% overhead:** {(valid_overheads < 5).sum()}/{len(valid_overheads)} ")
        f.write(f"({(valid_overheads < 5).sum() / len(valid_overheads) * 100:.1f}%)\n")
        f.write(f"- **Benchmarks with <10% overhead:** {(valid_overheads < 10).sum()}/{len(valid_overheads)} ")
        f.write(f"({(valid_overheads < 10).sum() / len(valid_overheads) * 100:.1f}%)\n")

    print(f"✓ Saved Markdown table: {markdown_file}")

    # Save JSON for visualizations
    json_file = output_path / 'analysis_data.json'
    analysis_data = {
        'summary_stats': summary_df.to_dict('records'),
        'overall_stats': {
            'optiweave': {
                'mean': float(valid_overheads.mean()),
                'median': float(valid_overheads.median()),
                'std': float(valid_overheads.std()),
                'min': float(valid_overheads.min()),
                'max': float(valid_overheads.max()),
                'q25': float(valid_overheads.quantile(0.25)),
                'q75': float(valid_overheads.quantile(0.75))
            },
            'gprof': {
                'mean': float(valid_gprof.mean()),
                'median': float(valid_gprof.median()),
                'std': float(valid_gprof.std()),
                'min': float(valid_gprof.min()),
                'max': float(valid_gprof.max()),
                'q25': float(valid_gprof.quantile(0.25)),
                'q75': float(valid_gprof.quantile(0.75))
            }
        },
        'category_distribution': category_counts.to_dict()
    }

    with open(json_file, 'w') as f:
        json.dump(analysis_data, f, indent=2, default=str)

    print(f"✓ Saved JSON data: {json_file}")
    print()

    # ========================================================================
    # 6. KEY FINDINGS
    # ========================================================================
    print("=" * 80)
    print("6. KEY FINDINGS FOR THESIS")
    print("=" * 80)
    print()

    print(f"1. OptiWeave array instrumentation adds {valid_overheads.mean():.1f}% overhead on average")
    print(f"   (median: {valid_overheads.median():.1f}%, range: {valid_overheads.min():.1f}%-{valid_overheads.max():.1f}%)")
    print()
    print(f"2. gprof adds {valid_gprof.mean():.1f}% overhead on average")
    print(f"   (median: {valid_gprof.median():.1f}%, range: {valid_gprof.min():.1f}%-{valid_gprof.max():.1f}%)")
    print()
    print(f"3. {(valid_overheads < 10).sum()}/{len(valid_overheads)} benchmarks ({(valid_overheads < 10).sum() / len(valid_overheads) * 100:.1f}%) have <10% overhead")
    print()
    print(f"4. Statistical comparison shows {'significant' if p_value < 0.05 else 'no significant'} difference")
    print(f"   between OptiWeave and gprof (p={p_value:.4f})")
    print()
    print(f"5. {significant_count} benchmarks show statistically significant overhead (α=0.05)")
    print()
    print("These results demonstrate that AST-level instrumentation provides")
    print("competitive overhead compared to traditional profiling tools like gprof,")
    print("answering RQ1 regarding the trade-off between instrumentation granularity")
    print("and runtime overhead.")
    print()
    print("=" * 80)

    return summary_df, analysis_data

if __name__ == '__main__':
    # Use expanded results if available, otherwise use standard results
    script_dir = Path(__file__).parent

    expanded_results = script_dir.parent / 'results' / 'rq1_expanded' / 'overhead_results.csv'
    standard_results = script_dir.parent / 'results' / 'rq1' / 'overhead_results.csv'

    if expanded_results.exists():
        results_path = expanded_results
        output_dir = expanded_results.parent / 'analysis'
        print(f"Using expanded results: {results_path}")
    elif standard_results.exists():
        results_path = standard_results
        output_dir = standard_results.parent / 'analysis'
        print(f"Using standard results: {results_path}")
    else:
        print("Error: No results file found!")
        print(f"Expected: {expanded_results}")
        print(f"      or: {standard_results}")
        sys.exit(1)

    print()
    summary_df, analysis_data = analyze_overhead(results_path, output_dir)
