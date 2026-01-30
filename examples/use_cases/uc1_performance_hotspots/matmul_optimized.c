/**
 * UC1: Performance Hotspot Detection Use Case - OPTIMIZED VERSION
 * 
 * After OptiWeave identified the hotspot in matmul_naive.c:
 * - Inner loop at line 24-27 showed highest array_subscript count
 * - B[k*n+j] access pattern identified as column-major (cache-unfriendly)
 * 
 * Optimization applied: Loop interchange + blocking for cache efficiency
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MATRIX_SIZE 512
#define BLOCK_SIZE 32  // Typical L1 cache line optimization

// Optimized: Loop interchange (i-k-j order for row-major access)
// This was identified by OptiWeave's access pattern analysis
void matmul_loop_interchange(double* A, double* B, double* C, int n) {
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            double a_ik = A[i * n + k];
            for (int j = 0; j < n; j++) {
                // Now both A and B are accessed in row-major order
                C[i * n + j] += a_ik * B[k * n + j];
            }
        }
    }
}

// Optimized: Cache blocking (tiling) for better locality
void matmul_blocked(double* A, double* B, double* C, int n) {
    int block = BLOCK_SIZE;
    
    for (int ii = 0; ii < n; ii += block) {
        for (int kk = 0; kk < n; kk += block) {
            for (int jj = 0; jj < n; jj += block) {
                // Compute block
                int i_max = (ii + block < n) ? ii + block : n;
                int k_max = (kk + block < n) ? kk + block : n;
                int j_max = (jj + block < n) ? jj + block : n;
                
                for (int i = ii; i < i_max; i++) {
                    for (int k = kk; k < k_max; k++) {
                        double a_ik = A[i * n + k];
                        for (int j = jj; j < j_max; j++) {
                            C[i * n + j] += a_ik * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
}

// Initialize matrix with random values
void init_matrix(double* M, int n) {
    for (int i = 0; i < n * n; i++) {
        M[i] = (double)rand() / RAND_MAX;
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
    double ci_low;
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
        times[i] = end - start;
    }
    
    // Calculate statistics
    BenchmarkResult result = {0};
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
    
    printf("=== UC1: Optimized Matrix Multiplication ===\n");
    printf("Matrix size: %d x %d, Block size: %d\n", n, n, BLOCK_SIZE);
    printf("Warmup runs: %d, Measured runs: %d\n\n", warmup, runs);
    
    // Allocate matrices
    double* A = (double*)malloc(n * n * sizeof(double));
    double* B = (double*)malloc(n * n * sizeof(double));
    double* C = (double*)malloc(n * n * sizeof(double));
    
    if (!A || !B || !C) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Initialize with same seed for fair comparison
    srand(42);
    init_matrix(A, n);
    init_matrix(B, n);
    
    // Calculate GFLOPS helper
    double flops = 2.0 * n * n * n;
    
    // Benchmark loop interchange
    printf("Running loop interchange optimization...\n");
    BenchmarkResult result_interchange = run_benchmark(matmul_loop_interchange, A, B, C, n, warmup, runs);
    double gflops_interchange = flops / result_interchange.mean / 1e9;
    
    // Benchmark blocked
    printf("Running cache blocking optimization...\n");
    BenchmarkResult result_blocked = run_benchmark(matmul_blocked, A, B, C, n, warmup, runs);
    double gflops_blocked = flops / result_blocked.mean / 1e9;
    
    // Report results
    printf("\n=== Results ===\n");
    printf("\nLoop Interchange (i-k-j order):\n");
    printf("  Mean:   %.4f seconds (%.2f GFLOPS)\n", result_interchange.mean, gflops_interchange);
    printf("  Stddev: %.4f seconds\n", result_interchange.stddev);
    printf("  95%% CI: [%.4f, %.4f] seconds\n", result_interchange.ci_low, result_interchange.ci_high);
    
    printf("\nCache Blocking (%dx%d blocks):\n", BLOCK_SIZE, BLOCK_SIZE);
    printf("  Mean:   %.4f seconds (%.2f GFLOPS)\n", result_blocked.mean, gflops_blocked);
    printf("  Stddev: %.4f seconds\n", result_blocked.stddev);
    printf("  95%% CI: [%.4f, %.4f] seconds\n", result_blocked.ci_low, result_blocked.ci_high);
    
    // CSV output
    printf("\n=== CSV Output ===\n");
    printf("implementation,size,mean,stddev,ci_low,ci_high,min,max,gflops\n");
    printf("loop_interchange,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
           n, result_interchange.mean, result_interchange.stddev,
           result_interchange.ci_low, result_interchange.ci_high,
           result_interchange.min, result_interchange.max, gflops_interchange);
    printf("blocked,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
           n, result_blocked.mean, result_blocked.stddev,
           result_blocked.ci_low, result_blocked.ci_high,
           result_blocked.min, result_blocked.max, gflops_blocked);
    
    free(A);
    free(B);
    free(C);
    
    return 0;
}
