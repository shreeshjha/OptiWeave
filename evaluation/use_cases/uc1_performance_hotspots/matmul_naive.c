#include <optiweave/prelude_c.h>
/**
 * UC1: Performance Hotspot Detection Use Case
 * 
 * This demonstrates how OptiWeave identifies performance hotspots
 * and guides optimization decisions.
 * 
 * Scenario: Naive matrix multiplication with suboptimal memory access
 * Goal: Use OptiWeave profiling to identify the hotspot, then optimize
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MATRIX_SIZE 512

// Naive matrix multiplication - poor cache utilization
// OptiWeave should identify the inner loop as a hotspot
void matmul_naive(double* A, double* B, double* C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                // This access pattern is cache-unfriendly for B
                // B[k][j] strides by 'n' elements (column access)
                sum += __ow_subscript_impl(A, i * n + k, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 28, __FUNCTION__) * __ow_subscript_impl(B, k * n + j, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 28, __FUNCTION__);
            }
            (__optiweave_record_subscript("/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 30, __FUNCTION__), C[i * n + j] = sum);
        }
    }
}

// Initialize matrix with random values
void init_matrix(double* M, int n) {
    for (int i = 0; i < n * n; i++) {
        (__optiweave_record_subscript("/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 38, __FUNCTION__), M[i] = (double)rand() / RAND_MAX);
    }
}

// Zero out matrix
void zero_matrix(double* M, int n) {
    memset(M, 0, n * n * sizeof(double));
}

// Get time in seconds
double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Benchmark with proper methodology
typedef struct {
    double mean;
    double stddev;
    double min;
    double max;
    double ci_low;   // 95% confidence interval
    double ci_high;
} BenchmarkResult;

BenchmarkResult run_benchmark(void (*matmul_func)(double*, double*, double*, int),
                              double* A, double* B, double* C, int n,
                              int warmup_runs, int measured_runs) {
    double* times = (double*)malloc(measured_runs * sizeof(double));
    
    // Warmup
    for (int i = 0; i < warmup_runs; i++) {
        zero_matrix(C, n);
        matmul_func(A, B, C, n);
    }
    
    // Measured runs
    for (int i = 0; i < measured_runs; i++) {
        zero_matrix(C, n);
        double start = get_time_sec();
        matmul_func(A, B, C, n);
        double end = get_time_sec();
        (__optiweave_record_subscript("/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 81, __FUNCTION__), times[i] = end - start);
    }
    
    // Calculate statistics
    BenchmarkResult result = {0};
    result.min = __ow_subscript_impl(times, 0, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 86, __FUNCTION__);
    result.max = __ow_subscript_impl(times, 0, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 87, __FUNCTION__);
    
    double sum = 0.0;
    for (int i = 0; i < measured_runs; i++) {
        sum += __ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 91, __FUNCTION__);
        if (__ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 92, __FUNCTION__) < result.min) result.min = __ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 92, __FUNCTION__);
        if (__ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 93, __FUNCTION__) > result.max) result.max = __ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 93, __FUNCTION__);
    }
    result.mean = sum / measured_runs;
    
    // Standard deviation
    double variance_sum = 0.0;
    for (int i = 0; i < measured_runs; i++) {
        double diff = __ow_subscript_impl(times, i, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 100, __FUNCTION__) - result.mean;
        variance_sum += diff * diff;
    }
    result.stddev = sqrt(variance_sum / (measured_runs - 1));
    
    // 95% confidence interval (t-value for 30 samples ~ 2.045)
    double t_value = 2.045;  // For 30 samples
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
    
    if (argc > 1) n = atoi(__ow_subscript_impl(argv, 1, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 120, __FUNCTION__));
    if (argc > 2) runs = atoi(__ow_subscript_impl(argv, 2, "/root/testing/OptiWeave/evaluation/use_cases/uc1_performance_hotspots/matmul_naive.c", 121, __FUNCTION__));
    
    printf("=== UC1: Performance Hotspot Detection ===\n");
    printf("Matrix size: %d x %d\n", n, n);
    printf("Warmup runs: %d, Measured runs: %d\n\n", warmup, runs);
    
    // Allocate matrices
    double* A = (double*)malloc(n * n * sizeof(double));
    double* B = (double*)malloc(n * n * sizeof(double));
    double* C = (double*)malloc(n * n * sizeof(double));
    
    if (!A || !B || !C) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Initialize
    srand(42);  // Fixed seed for reproducibility
    init_matrix(A, n);
    init_matrix(B, n);
    
    // Run benchmark
    printf("Running naive matrix multiplication...\n");
    BenchmarkResult result = run_benchmark(matmul_naive, A, B, C, n, warmup, runs);
    
    // Report results
    printf("\n=== Results (Naive Implementation) ===\n");
    printf("Mean:   %.4f seconds\n", result.mean);
    printf("Stddev: %.4f seconds\n", result.stddev);
    printf("95%% CI: [%.4f, %.4f] seconds\n", result.ci_low, result.ci_high);
    printf("Min:    %.4f seconds\n", result.min);
    printf("Max:    %.4f seconds\n", result.max);
    
    // Calculate GFLOPS
    double flops = 2.0 * n * n * n;  // 2n^3 FLOPs for matrix multiply
    double gflops = flops / result.mean / 1e9;
    printf("Performance: %.2f GFLOPS\n", gflops);
    
    // CSV output for automated analysis
    printf("\n=== CSV Output ===\n");
    printf("implementation,size,mean,stddev,ci_low,ci_high,min,max,gflops\n");
    printf("naive,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
           n, result.mean, result.stddev, result.ci_low, result.ci_high,
           result.min, result.max, gflops);
    
    free(A);
    free(B);
    free(C);
    
    return 0;
}
