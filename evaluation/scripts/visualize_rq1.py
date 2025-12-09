#!/usr/bin/env python3
"""
RQ1 Visualization Script
Creates publication-quality graphs for thesis
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import json
from pathlib import Path
import sys

# Set publication-quality defaults
plt.rcParams['figure.dpi'] = 300
plt.rcParams['savefig.dpi'] = 300
plt.rcParams['font.size'] = 10
plt.rcParams['font.family'] = 'serif'
plt.rcParams['figure.figsize'] = (8, 6)

def load_data(analysis_dir):
    """Load analysis data"""
    json_path = analysis_dir / 'analysis_data.json'
    csv_path = analysis_dir / 'comprehensive_summary.csv'

    with open(json_path, 'r') as f:
        json_data = json.load(f)

    df = pd.read_csv(csv_path)

    # Filter out failed benchmarks for visualizations
    df_valid = df[df['optiweave_overhead_pct'].notna()].copy()

    return df_valid, json_data

def plot_overhead_comparison(df, output_dir):
    """Bar chart comparing OptiWeave vs gprof overhead"""
    fig, ax = plt.subplots(figsize=(12, 6))

    # Sort by OptiWeave overhead
    df_sorted = df.sort_values('optiweave_overhead_pct')

    x = np.arange(len(df_sorted))
    width = 0.35

    bars1 = ax.bar(x - width/2, df_sorted['optiweave_overhead_pct'],
                   width, label='OptiWeave', color='#2E86AB', alpha=0.8)
    bars2 = ax.bar(x + width/2, df_sorted['gprof_overhead_pct'],
                   width, label='gprof', color='#A23B72', alpha=0.8)

    # Add horizontal line at 0% and 10%
    ax.axhline(y=0, color='gray', linestyle='-', linewidth=0.8, alpha=0.5)
    ax.axhline(y=10, color='red', linestyle='--', linewidth=0.8, alpha=0.3, label='10% threshold')

    # Customize
    ax.set_xlabel('Benchmark', fontweight='bold')
    ax.set_ylabel('Overhead (%)', fontweight='bold')
    ax.set_title('Runtime Overhead Comparison: OptiWeave vs gprof', fontweight='bold', pad=20)
    ax.set_xticks(x)
    ax.set_xticklabels(df_sorted['benchmark'], rotation=45, ha='right', fontsize=8)
    ax.legend(loc='upper left')
    ax.grid(axis='y', alpha=0.3)

    # Color bars based on significance
    for i, (idx, row) in enumerate(df_sorted.iterrows()):
        if row['significant'] == 'Yes':
            bars1[i].set_edgecolor('black')
            bars1[i].set_linewidth(1.5)

    plt.tight_layout()
    plt.savefig(output_dir / 'overhead_comparison_bar.png', bbox_inches='tight')
    plt.savefig(output_dir / 'overhead_comparison_bar.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'overhead_comparison_bar.png'}")
    plt.close()

def plot_box_plot(df, output_dir):
    """Box plot showing overhead distribution"""
    fig, ax = plt.subplots(figsize=(8, 6))

    # Prepare data
    data_to_plot = [
        df['optiweave_overhead_pct'].dropna(),
        df['gprof_overhead_pct'].dropna()
    ]

    bp = ax.boxplot(data_to_plot, labels=['OptiWeave', 'gprof'],
                    patch_artist=True, widths=0.6)

    # Customize colors
    colors = ['#2E86AB', '#A23B72']
    for patch, color in zip(bp['boxes'], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.7)

    # Add mean markers
    means = [d.mean() for d in data_to_plot]
    ax.scatter([1, 2], means, color='red', marker='D', s=100,
               zorder=3, label='Mean')

    # Add horizontal line at 0% and 10%
    ax.axhline(y=0, color='gray', linestyle='-', linewidth=0.8, alpha=0.5)
    ax.axhline(y=10, color='red', linestyle='--', linewidth=0.8,
               alpha=0.3, label='10% threshold')

    ax.set_ylabel('Overhead (%)', fontweight='bold')
    ax.set_title('Distribution of Runtime Overhead', fontweight='bold', pad=20)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_dir / 'overhead_distribution_box.png', bbox_inches='tight')
    plt.savefig(output_dir / 'overhead_distribution_box.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'overhead_distribution_box.png'}")
    plt.close()

def plot_scatter_comparison(df, output_dir):
    """Scatter plot: OptiWeave vs gprof overhead"""
    fig, ax = plt.subplots(figsize=(8, 8))

    # Color by significance
    colors = df['significant'].map({'Yes': '#E63946', 'No': '#457B9D'})

    scatter = ax.scatter(df['gprof_overhead_pct'], df['optiweave_overhead_pct'],
                        c=colors, alpha=0.7, s=100, edgecolors='black', linewidth=0.5)

    # Add diagonal line (equal overhead)
    min_val = min(df['gprof_overhead_pct'].min(), df['optiweave_overhead_pct'].min())
    max_val = max(df['gprof_overhead_pct'].max(), df['optiweave_overhead_pct'].max())
    ax.plot([min_val, max_val], [min_val, max_val], 'k--', alpha=0.3, linewidth=1)

    # Add reference lines
    ax.axhline(y=10, color='red', linestyle='--', linewidth=0.8, alpha=0.3)
    ax.axvline(x=10, color='red', linestyle='--', linewidth=0.8, alpha=0.3)

    # Labels for interesting points
    for _, row in df.iterrows():
        if abs(row['optiweave_overhead_pct'] - row['gprof_overhead_pct']) > 15:
            ax.annotate(row['benchmark'],
                       (row['gprof_overhead_pct'], row['optiweave_overhead_pct']),
                       fontsize=7, alpha=0.7, xytext=(5, 5),
                       textcoords='offset points')

    ax.set_xlabel('gprof Overhead (%)', fontweight='bold')
    ax.set_ylabel('OptiWeave Overhead (%)', fontweight='bold')
    ax.set_title('OptiWeave vs gprof: Overhead Comparison', fontweight='bold', pad=20)
    ax.grid(alpha=0.3)

    # Legend
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#E63946', label='Statistically Significant'),
        Patch(facecolor='#457B9D', label='Not Significant')
    ]
    ax.legend(handles=legend_elements, loc='upper left')

    plt.tight_layout()
    plt.savefig(output_dir / 'overhead_scatter.png', bbox_inches='tight')
    plt.savefig(output_dir / 'overhead_scatter.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'overhead_scatter.png'}")
    plt.close()

def plot_category_distribution(df, output_dir):
    """Pie chart showing overhead categories"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Categorize overhead
    def categorize(overhead):
        if pd.isna(overhead):
            return "Failed"
        elif overhead < 0:
            return "Speedup (<0%)"
        elif overhead < 5:
            return "Excellent (<5%)"
        elif overhead < 10:
            return "Good (5-10%)"
        elif overhead < 20:
            return "Acceptable (10-20%)"
        else:
            return "High (>20%)"

    df['category'] = df['optiweave_overhead_pct'].apply(categorize)
    category_counts = df['category'].value_counts()

    # OptiWeave pie chart
    colors_pie = ['#06D6A0', '#118AB2', '#073B4C', '#FFD166', '#EF476F', '#9D0208']
    explode = [0.05 if cat in ['High (>20%)', 'Failed'] else 0 for cat in category_counts.index]

    wedges, texts, autotexts = ax1.pie(category_counts, labels=category_counts.index,
                                        autopct='%1.1f%%', startangle=90,
                                        colors=colors_pie[:len(category_counts)],
                                        explode=explode)

    for autotext in autotexts:
        autotext.set_color('white')
        autotext.set_fontweight('bold')

    ax1.set_title('OptiWeave Overhead Distribution', fontweight='bold', pad=20)

    # Summary statistics as text
    stats_text = f"""Summary Statistics:

Mean:     {df['optiweave_overhead_pct'].mean():.2f}%
Median:   {df['optiweave_overhead_pct'].median():.2f}%
Std Dev:  {df['optiweave_overhead_pct'].std():.2f}%

Benchmarks < 10%:
{(df['optiweave_overhead_pct'] < 10).sum()}/{len(df)} ({(df['optiweave_overhead_pct'] < 10).sum() / len(df) * 100:.1f}%)

Statistically Significant:
{(df['significant'] == 'Yes').sum()}/{len(df)} ({(df['significant'] == 'Yes').sum() / len(df) * 100:.1f}%)
"""

    ax2.text(0.1, 0.5, stats_text, fontsize=11, verticalalignment='center',
             fontfamily='monospace', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))
    ax2.axis('off')

    plt.tight_layout()
    plt.savefig(output_dir / 'overhead_categories.png', bbox_inches='tight')
    plt.savefig(output_dir / 'overhead_categories.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'overhead_categories.png'}")
    plt.close()

def plot_histogram(df, output_dir):
    """Histogram of overhead distribution"""
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot both distributions
    ax.hist(df['optiweave_overhead_pct'].dropna(), bins=20, alpha=0.6,
            label='OptiWeave', color='#2E86AB', edgecolor='black')
    ax.hist(df['gprof_overhead_pct'].dropna(), bins=20, alpha=0.6,
            label='gprof', color='#A23B72', edgecolor='black')

    # Add vertical lines for means
    ax.axvline(df['optiweave_overhead_pct'].mean(), color='#2E86AB',
               linestyle='--', linewidth=2, label=f'OptiWeave Mean: {df["optiweave_overhead_pct"].mean():.1f}%')
    ax.axvline(df['gprof_overhead_pct'].mean(), color='#A23B72',
               linestyle='--', linewidth=2, label=f'gprof Mean: {df["gprof_overhead_pct"].mean():.1f}%')

    # Add reference line at 10%
    ax.axvline(10, color='red', linestyle=':', linewidth=2, alpha=0.5, label='10% threshold')

    ax.set_xlabel('Overhead (%)', fontweight='bold')
    ax.set_ylabel('Frequency', fontweight='bold')
    ax.set_title('Overhead Distribution Histogram', fontweight='bold', pad=20)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_dir / 'overhead_histogram.png', bbox_inches='tight')
    plt.savefig(output_dir / 'overhead_histogram.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'overhead_histogram.png'}")
    plt.close()

def plot_runtime_comparison(df, output_dir):
    """Stacked bar showing absolute runtimes"""
    fig, ax = plt.subplots(figsize=(12, 6))

    # Sort by baseline runtime
    df_sorted = df.sort_values('baseline_mean')

    x = np.arange(len(df_sorted))
    width = 0.6

    # Plot baseline and additional time
    baseline_bars = ax.bar(x, df_sorted['baseline_mean'], width,
                          label='Baseline Runtime', color='#118AB2', alpha=0.8)

    overhead_time = df_sorted['optiweave_mean'] - df_sorted['baseline_mean']
    overhead_bars = ax.bar(x, overhead_time, width, bottom=df_sorted['baseline_mean'],
                          label='Instrumentation Overhead', color='#EF476F', alpha=0.8)

    ax.set_xlabel('Benchmark', fontweight='bold')
    ax.set_ylabel('Runtime (seconds)', fontweight='bold')
    ax.set_title('Absolute Runtime: Baseline vs Instrumented', fontweight='bold', pad=20)
    ax.set_xticks(x)
    ax.set_xticklabels(df_sorted['benchmark'], rotation=45, ha='right', fontsize=8)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_dir / 'runtime_absolute.png', bbox_inches='tight')
    plt.savefig(output_dir / 'runtime_absolute.pdf', bbox_inches='tight')
    print(f"✓ Saved: {output_dir / 'runtime_absolute.png'}")
    plt.close()

def create_all_visualizations(analysis_dir, output_dir):
    """Create all visualizations"""
    print("=" * 80)
    print("GENERATING VISUALIZATIONS FOR RQ1")
    print("=" * 80)
    print()

    # Load data
    df, json_data = load_data(analysis_dir)
    print(f"Loaded data for {len(df)} valid benchmarks")
    print()

    # Create output directory
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    print("Creating visualizations...")
    print()

    # Generate all plots
    plot_overhead_comparison(df, output_path)
    plot_box_plot(df, output_path)
    plot_scatter_comparison(df, output_path)
    plot_category_distribution(df, output_path)
    plot_histogram(df, output_path)
    plot_runtime_comparison(df, output_path)

    print()
    print("=" * 80)
    print("VISUALIZATION COMPLETE")
    print("=" * 80)
    print()
    print(f"All visualizations saved to: {output_path}")
    print()
    print("Generated files:")
    print("  • overhead_comparison_bar.png/pdf    - Bar chart comparison")
    print("  • overhead_distribution_box.png/pdf  - Box plot distribution")
    print("  • overhead_scatter.png/pdf           - Scatter plot comparison")
    print("  • overhead_categories.png/pdf        - Category pie chart")
    print("  • overhead_histogram.png/pdf         - Histogram distribution")
    print("  • runtime_absolute.png/pdf           - Absolute runtime comparison")
    print()
    print("Use these in your thesis Chapter 5.1 (Results)")
    print("=" * 80)

if __name__ == '__main__':
    script_dir = Path(__file__).parent

    # Check for expanded results
    expanded_analysis = script_dir.parent / 'results' / 'rq1_expanded' / 'analysis'
    standard_analysis = script_dir.parent / 'results' / 'rq1' / 'analysis'

    if expanded_analysis.exists():
        analysis_dir = expanded_analysis
        output_dir = expanded_analysis / 'visualizations'
        print(f"Using expanded analysis: {analysis_dir}")
    elif standard_analysis.exists():
        analysis_dir = standard_analysis
        output_dir = standard_analysis / 'visualizations'
        print(f"Using standard analysis: {analysis_dir}")
    else:
        print("Error: No analysis data found!")
        print("Run comprehensive_analyze_rq1.py first")
        sys.exit(1)

    print()
    create_all_visualizations(analysis_dir, output_dir)
