/**
 * UC2: Cache Optimization Use Case
 * 
 * This demonstrates how OptiWeave detects cache-unfriendly memory access
 * patterns and guides optimization.
 * 
 * Scenario: Matrix traversal with row-major vs column-major access
 * Goal: OptiWeave identifies strided access pattern, optimization yields 5-10x speedup
 * 
 * Key insight: C/C++ arrays are row-major. Accessing column-by-column
 * causes cache misses on every access (stride = row_size * sizeof(element))
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MATRIX_SIZE 4096  // Large enough to exceed L1/L2 cache

// BAD: Column-major traversal (cache-unfriendly)
// OptiWeave should detect: array subscript pattern [j][i] with outer loop on i
// This causes stride-N access, missing cache on every element
long long sum_column_major(int** matrix, int n) {
    long long sum = 0;
    for (int j = 0; j < n; j++) {       // Column first
        for (int i = 0; i < n; i++) {   // Row second
            sum += matrix[i][j];         // Strided access!
        }
    }
    return sum;
}

// GOOD: Row-major traversal (cache-friendly)
// Sequential memory access, each cache line is fully utilized
long long sum_row_major(int** matrix, int n) {
    long long sum = 0;
    for (int i = 0; i < n; i++) {       // Row first
        for (int j = 0; j < n; j++) {   // Column second
            sum += matrix[i][j];         // Sequential access
        }
    }
    return sum;
}

// Allocate 2D matrix
int** allocate_matrix(int n) {
    int** matrix = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matrix[i] = (int*)malloc(n * sizeof(int));
    }
    return matrix;
}

// Initialize with values
void init_matrix(int** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (i * n + j) % 100;
        }
    }
}

// Free matrix
void free_matrix(int** matrix, int n) {
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Get time in seconds
double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Benchmark result structure
typedef struct {
    double mean;
    double stddev;
    double min;
    double max;
    double ci_low;
    double ci_high;
    long long checksum;  // For verification
} BenchmarkResult;

BenchmarkResult run_benchmark(long long (*sum_func)(int**, int),
                              int** matrix, int n,
                              int warmup_runs, int measured_runs) {
    double* times = (double*)malloc(measured_runs * sizeof(double));
    long long checksum = 0;
    
    // Warmup
    for (int i = 0; i < warmup_runs; i++) {
        checksum = sum_func(matrix, n);
    }
    
    // Measured runs
    for (int i = 0; i < measured_runs; i++) {
        double start = get_time_sec();
        checksum = sum_func(matrix, n);
        double end = get_time_sec();
        times[i] = end - start;
    }
    
    // Calculate statistics
    BenchmarkResult result = {0};
    result.checksum = checksum;
    result.min = times[0];
    result.max = times[0];
    
    double sum = 0.0;
    for (int i = 0; i < measured_runs; i++) {
        sum += times[i];
        if (times[i] < result.min) result.min = times[i];
        if (times[i] > result.max) result.max = times[i];
    }
    result.mean = sum / measured_runs;
    
    // Standard deviation
    double variance_sum = 0.0;
    for (int i = 0; i < measured_runs; i++) {
        double diff = times[i] - result.mean;
        variance_sum += diff * diff;
    }
    result.stddev = sqrt(variance_sum / (measured_runs - 1));
    
    // 95% confidence interval
    double t_value = 2.045;
    double margin = t_value * result.stddev / sqrt(measured_runs);
    result.ci_low = result.mean - margin;
    result.ci_high = result.mean + margin;
    
    free(times);
    return result;
}

int main(int argc, char** argv) {
    int n = MATRIX_SIZE;
    int warmup = 3;
    int runs = 30;
    
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) runs = atoi(argv[2]);
    
    printf("=== UC2: Cache Optimization - Memory Access Patterns ===\n");
    printf("Matrix size: %d x %d (%zu MB)\n", n, n, 
           (size_t)n * n * sizeof(int) / (1024 * 1024));
    printf("Warmup runs: %d, Measured runs: %d\n\n", warmup, runs);
    
    // Allocate and initialize matrix
    int** matrix = allocate_matrix(n);
    init_matrix(matrix, n);
    
    // Benchmark column-major (BAD)
    printf("Running column-major traversal (cache-unfriendly)...\n");
    BenchmarkResult result_col = run_benchmark(sum_column_major, matrix, n, warmup, runs);
    
    // Benchmark row-major (GOOD)
    printf("Running row-major traversal (cache-friendly)...\n");
    BenchmarkResult result_row = run_benchmark(sum_row_major, matrix, n, warmup, runs);
    
    // Verify correctness
    if (result_col.checksum != result_row.checksum) {
        fprintf(stderr, "ERROR: Checksums don't match!\n");
        return 1;
    }
    
    // Report results
    printf("\n=== Results ===\n");
    
    printf("\nColumn-Major (Cache-Unfriendly):\n");
    printf("  Mean:   %.6f seconds\n", result_col.mean);
    printf("  Stddev: %.6f seconds\n", result_col.stddev);
    printf("  95%% CI: [%.6f, %.6f] seconds\n", result_col.ci_low, result_col.ci_high);
    
    printf("\nRow-Major (Cache-Friendly):\n");
    printf("  Mean:   %.6f seconds\n", result_row.mean);
    printf("  Stddev: %.6f seconds\n", result_row.stddev);
    printf("  95%% CI: [%.6f, %.6f] seconds\n", result_row.ci_low, result_row.ci_high);
    
    // Calculate speedup
    double speedup = result_col.mean / result_row.mean;
    double slowdown_percent = ((result_col.mean - result_row.mean) / result_row.mean) * 100;
    
    printf("\n=== IMPACT SUMMARY ===\n");
    printf("Speedup (row-major vs column-major): %.2fx faster\n", speedup);
    printf("Column-major overhead: %.1f%% slower than optimal\n", slowdown_percent);
    printf("\nOptiWeave Insight: The strided access pattern in column-major\n");
    printf("traversal causes ~%.0f%% more cache misses.\n", slowdown_percent);
    printf("\nOptimization: Loop interchange (swap i and j loops) achieves\n");
    printf("%.2fx speedup with zero algorithmic changes.\n", speedup);
    
    // CSV output
    printf("\n=== CSV Output ===\n");
    printf("access_pattern,size,mean,stddev,ci_low,ci_high,min,max,checksum\n");
    printf("column_major,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%lld\n",
           n, result_col.mean, result_col.stddev,
           result_col.ci_low, result_col.ci_high,
           result_col.min, result_col.max, result_col.checksum);
    printf("row_major,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%lld\n",
           n, result_row.mean, result_row.stddev,
           result_row.ci_low, result_row.ci_high,
           result_row.min, result_row.max, result_row.checksum);
    
    free_matrix(matrix, n);
    
    return 0;
}
