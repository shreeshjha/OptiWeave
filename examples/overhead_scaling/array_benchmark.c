/*
 * Array Operations Benchmark for Overhead Scaling Study
 * Measures OptiWeave overhead across different workload sizes and iteration counts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Measure execution time in nanoseconds
static long long measure_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Workload 1: Array sum (read-heavy)
long long array_sum(int *arr, size_t size) {
    long long sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

// Workload 2: Array copy (read-write)
void array_copy(int *dest, const int *src, size_t size) {
    for (size_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

// Workload 3: Array transformation (compute + write)
void array_transform(int *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        arr[i] = arr[i] * 2 + 1;
    }
}

// Workload 4: Matrix multiplication (nested loops)
void matrix_multiply(int *result, const int *a, const int *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            int sum = 0;
            for (size_t k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            result[i * n + j] = sum;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <array_size> <iterations>\n", argv[0]);
        fprintf(stderr, "Example: %s 1000 10000\n", argv[0]);
        return 1;
    }

    size_t array_size = (size_t)atoi(argv[1]);
    int iterations = atoi(argv[2]);

    printf("Overhead Scaling Benchmark\n");
    printf("==========================\n");
    printf("Array size: %zu elements\n", array_size);
    printf("Iterations: %d\n", iterations);
    printf("Memory: %.2f KB per array\n", (array_size * sizeof(int)) / 1024.0);
    printf("\n");

    // Allocate arrays
    int *arr1 = (int*)malloc(array_size * sizeof(int));
    int *arr2 = (int*)malloc(array_size * sizeof(int));
    int *arr3 = (int*)malloc(array_size * sizeof(int));

    if (!arr1 || !arr2 || !arr3) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize arrays
    for (size_t i = 0; i < array_size; i++) {
        arr1[i] = (int)i;
        arr2[i] = (int)(i * 2);
        arr3[i] = 0;
    }

    // Warmup
    for (int i = 0; i < 100; i++) {
        array_sum(arr1, array_size);
    }

    printf("Running workloads...\n\n");

    // Workload 1: Array sum (read-heavy)
    printf("[1/4] Array Sum (read-heavy)\n");
    long long start = measure_time_ns();
    long long checksum = 0;
    for (int i = 0; i < iterations; i++) {
        checksum += array_sum(arr1, array_size);
    }
    long long end = measure_time_ns();
    double sum_time_ms = (end - start) / 1000000.0;
    printf("  Time: %.2f ms\n", sum_time_ms);
    printf("  Avg: %.2f μs per iteration\n", sum_time_ms * 1000.0 / iterations);
    printf("  Checksum: %lld\n\n", checksum);

    // Workload 2: Array copy (read-write)
    printf("[2/4] Array Copy (read-write)\n");
    start = measure_time_ns();
    for (int i = 0; i < iterations; i++) {
        array_copy(arr3, arr1, array_size);
    }
    end = measure_time_ns();
    double copy_time_ms = (end - start) / 1000000.0;
    printf("  Time: %.2f ms\n", copy_time_ms);
    printf("  Avg: %.2f μs per iteration\n", copy_time_ms * 1000.0 / iterations);
    printf("  Last element: %d\n\n", arr3[array_size-1]);

    // Workload 3: Array transformation (compute + write)
    printf("[3/4] Array Transform (compute + write)\n");
    memcpy(arr3, arr1, array_size * sizeof(int));  // Reset arr3
    start = measure_time_ns();
    for (int i = 0; i < iterations; i++) {
        array_transform(arr3, array_size);
    }
    end = measure_time_ns();
    double transform_time_ms = (end - start) / 1000000.0;
    printf("  Time: %.2f ms\n", transform_time_ms);
    printf("  Avg: %.2f μs per iteration\n", transform_time_ms * 1000.0 / iterations);
    printf("  Last element: %d\n\n", arr3[array_size-1]);

    // Workload 4: Matrix multiply (for smaller sizes only)
    if (array_size <= 100) {
        size_t matrix_size = (size_t)sqrt((double)array_size);
        if (matrix_size * matrix_size <= array_size) {
            printf("[4/4] Matrix Multiply %zux%zu\n", matrix_size, matrix_size);
            int matrix_iters = iterations / 10;  // Fewer iterations for expensive operation
            if (matrix_iters < 1) matrix_iters = 1;

            start = measure_time_ns();
            for (int i = 0; i < matrix_iters; i++) {
                matrix_multiply(arr3, arr1, arr2, matrix_size);
            }
            end = measure_time_ns();
            double matmul_time_ms = (end - start) / 1000000.0;
            printf("  Time: %.2f ms (%d iterations)\n", matmul_time_ms, matrix_iters);
            printf("  Avg: %.2f μs per iteration\n", matmul_time_ms * 1000.0 / matrix_iters);
            printf("  Result[0][0]: %d\n\n", arr3[0]);
        }
    } else {
        printf("[4/4] Matrix Multiply: Skipped (array too large)\n\n");
    }

    // Summary
    printf("==========================\n");
    printf("Total array operations: ~%zu per iteration\n", array_size * 3);
    printf("Total time: %.2f ms\n", sum_time_ms + copy_time_ms + transform_time_ms);
    printf("\n");

    free(arr1);
    free(arr2);
    free(arr3);

    return 0;
}
