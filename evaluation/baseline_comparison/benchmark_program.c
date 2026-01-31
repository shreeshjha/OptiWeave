/**
 * OptiWeave Baseline Comparison Benchmark
 * 
 * This program serves as a consistent benchmark for comparing profiling tools:
 * - OptiWeave (our tool)
 * - gprof (GNU profiler)
 * - Valgrind (memory analysis)
 * - perf (Linux performance counters)
 * 
 * The workloads are designed to stress different aspects:
 * 1. Array access patterns (cache behavior)
 * 2. Arithmetic operations (CPU-bound)
 * 3. Memory allocation (heap operations)
 * 4. Function call overhead
 * 5. Mixed workload (realistic scenario)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Configuration
#define ARRAY_SIZE 10000
#define ITERATIONS 1000
#define MATRIX_SIZE 200
#define ALLOC_COUNT 10000
#define ALLOC_SIZE 1024

// Prevent compiler optimization
volatile int sink = 0;

// ============================================================================
// Workload 1: Array Access Patterns (tests cache behavior)
// ============================================================================
void workload_sequential_access(int* arr, int size) {
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < size; i++) {
            sink += arr[i];
        }
    }
}

void workload_strided_access(int* arr, int size) {
    // Stride of 16 to cause cache misses
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < size; i += 16) {
            sink += arr[i];
        }
    }
}

void workload_random_access(int* arr, int* indices, int size) {
    for (int iter = 0; iter < ITERATIONS / 10; iter++) {
        for (int i = 0; i < size; i++) {
            sink += arr[indices[i]];
        }
    }
}

// ============================================================================
// Workload 2: Arithmetic Operations (CPU-bound)
// ============================================================================
void workload_integer_arithmetic(void) {
    int a = 1, b = 2, c = 3;
    for (int i = 0; i < ITERATIONS * 10000; i++) {
        int divisor_c = (c % 100) + 1;  // Ensure never zero
        int divisor_b = (abs(b) % 100) + 1;  // Ensure never zero
        a = (a + b) * (c % 10 + 1) - (b / divisor_c);
        b = ((b * a) % divisor_c) + 1;
        c = ((c + a) / divisor_b) + 1;
        // Keep values bounded
        a = (a % 10000) + 1;
        b = (b % 10000) + 1;
        c = (c % 10000) + 1;
        sink = a + b + c;
    }
}

void workload_floating_point(void) {
    double a = 1.5, b = 2.7, c = 3.14159;
    for (int i = 0; i < ITERATIONS * 5000; i++) {
        a = sin(a) + cos(b) * c;
        b = sqrt(fabs(a * b) + 0.1) + 1.0;
        c = log(fabs(a) + 1.0) * (1.0 + fmod(b, 10.0) / 100.0);
        // Keep values bounded to prevent overflow
        if (a > 1e10) a = 1.5;
        if (b > 1e10) b = 2.7;
        if (c > 1e10) c = 3.14159;
        sink = (int)(fmod(a + b + c, 1000.0));
    }
}

// ============================================================================
// Workload 3: Matrix Operations (cache + arithmetic)
// ============================================================================
void workload_matrix_multiply(double** A, double** B, double** C, int n) {
    // Naive matrix multiplication - O(n^3)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void workload_matrix_transpose(double** M, double** T, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            T[j][i] = M[i][j];
        }
    }
}

// ============================================================================
// Workload 4: Memory Allocation (heap operations)
// ============================================================================
void workload_malloc_free(void) {
    void* ptrs[100];
    for (int iter = 0; iter < ALLOC_COUNT / 100; iter++) {
        // Allocate
        for (int i = 0; i < 100; i++) {
            ptrs[i] = malloc(ALLOC_SIZE);
            memset(ptrs[i], iter & 0xFF, ALLOC_SIZE);
        }
        // Free in reverse order
        for (int i = 99; i >= 0; i--) {
            free(ptrs[i]);
        }
    }
}

void workload_realloc_pattern(void) {
    for (int iter = 0; iter < ALLOC_COUNT / 10; iter++) {
        void* ptr = malloc(64);
        for (int size = 128; size <= 4096; size *= 2) {
            ptr = realloc(ptr, size);
            memset(ptr, iter & 0xFF, size);
        }
        free(ptr);
    }
}

// ============================================================================
// Workload 5: Function Calls (call overhead)
// ============================================================================
int recursive_fibonacci(int n) {
    if (n <= 1) return n;
    return recursive_fibonacci(n - 1) + recursive_fibonacci(n - 2);
}

int iterative_sum(int* arr, int start, int end) {
    int sum = 0;
    for (int i = start; i < end; i++) {
        sum += arr[i];
    }
    return sum;
}

void workload_function_calls(int* arr, int size) {
    // Mix of recursive and iterative calls
    for (int i = 0; i < 100; i++) {
        sink += recursive_fibonacci(20);
    }
    
    for (int iter = 0; iter < ITERATIONS * 10; iter++) {
        sink += iterative_sum(arr, 0, size / 4);
        sink += iterative_sum(arr, size / 4, size / 2);
        sink += iterative_sum(arr, size / 2, 3 * size / 4);
        sink += iterative_sum(arr, 3 * size / 4, size);
    }
}

// ============================================================================
// Workload 6: String Operations
// ============================================================================
void workload_string_ops(void) {
    char buffer[1024];
    char src[256] = "The quick brown fox jumps over the lazy dog. ";
    
    for (int iter = 0; iter < ITERATIONS * 100; iter++) {
        strcpy(buffer, src);
        strcat(buffer, src);
        strcat(buffer, src);
        sink += strlen(buffer);
        
        char* found = strstr(buffer, "fox");
        if (found) sink += (int)(found - buffer);
    }
}

// ============================================================================
// Utility Functions
// ============================================================================
double** allocate_matrix(int n) {
    double** M = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        M[i] = (double*)malloc(n * sizeof(double));
    }
    return M;
}

void free_matrix(double** M, int n) {
    for (int i = 0; i < n; i++) {
        free(M[i]);
    }
    free(M);
}

void init_matrix(double** M, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            M[i][j] = (double)(rand() % 100) / 10.0;
        }
    }
}

void shuffle_array(int* arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// ============================================================================
// Timing Utilities
// ============================================================================
typedef struct {
    struct timespec start;
    struct timespec end;
} Timer;

void timer_start(Timer* t) {
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

double timer_stop(Timer* t) {
    clock_gettime(CLOCK_MONOTONIC, &t->end);
    double elapsed = (t->end.tv_sec - t->start.tv_sec) * 1000.0;
    elapsed += (t->end.tv_nsec - t->start.tv_nsec) / 1000000.0;
    return elapsed;
}

// ============================================================================
// Main Benchmark Runner
// ============================================================================
int main(int argc, char* argv[]) {
    int run_all = 1;
    int run_workload = -1;
    
    if (argc > 1) {
        run_workload = atoi(argv[1]);
        run_all = 0;
    }
    
    printf("OptiWeave Baseline Comparison Benchmark\n");
    printf("========================================\n\n");
    
    srand(42);  // Deterministic results
    
    // Setup data structures
    int* arr = (int*)malloc(ARRAY_SIZE * sizeof(int));
    int* indices = (int*)malloc(ARRAY_SIZE * sizeof(int));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        arr[i] = rand() % 1000;
        indices[i] = i;
    }
    shuffle_array(indices, ARRAY_SIZE);
    
    double** A = allocate_matrix(MATRIX_SIZE);
    double** B = allocate_matrix(MATRIX_SIZE);
    double** C = allocate_matrix(MATRIX_SIZE);
    init_matrix(A, MATRIX_SIZE);
    init_matrix(B, MATRIX_SIZE);
    
    Timer timer;
    double elapsed;
    
    printf("Configuration:\n");
    printf("  Array size: %d\n", ARRAY_SIZE);
    printf("  Iterations: %d\n", ITERATIONS);
    printf("  Matrix size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("\n");
    
    // Workload 1: Sequential Array Access
    if (run_all || run_workload == 1) {
        printf("[1/8] Sequential Array Access...\n");
        timer_start(&timer);
        workload_sequential_access(arr, ARRAY_SIZE);
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 2: Strided Array Access
    if (run_all || run_workload == 2) {
        printf("[2/8] Strided Array Access...\n");
        timer_start(&timer);
        workload_strided_access(arr, ARRAY_SIZE);
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 3: Random Array Access
    if (run_all || run_workload == 3) {
        printf("[3/8] Random Array Access...\n");
        timer_start(&timer);
        workload_random_access(arr, indices, ARRAY_SIZE);
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 4: Integer Arithmetic
    if (run_all || run_workload == 4) {
        printf("[4/8] Integer Arithmetic...\n");
        timer_start(&timer);
        workload_integer_arithmetic();
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 5: Floating Point
    if (run_all || run_workload == 5) {
        printf("[5/8] Floating Point Operations...\n");
        timer_start(&timer);
        workload_floating_point();
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 6: Matrix Multiply
    if (run_all || run_workload == 6) {
        printf("[6/8] Matrix Multiplication (%dx%d)...\n", MATRIX_SIZE, MATRIX_SIZE);
        timer_start(&timer);
        workload_matrix_multiply(A, B, C, MATRIX_SIZE);
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 7: Memory Allocation
    if (run_all || run_workload == 7) {
        printf("[7/8] Memory Allocation...\n");
        timer_start(&timer);
        workload_malloc_free();
        workload_realloc_pattern();
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    // Workload 8: Function Calls
    if (run_all || run_workload == 8) {
        printf("[8/8] Function Calls...\n");
        timer_start(&timer);
        workload_function_calls(arr, ARRAY_SIZE);
        elapsed = timer_stop(&timer);
        printf("      Time: %.2f ms\n\n", elapsed);
    }
    
    printf("Benchmark complete. Sink value: %d\n", sink);
    
    // Cleanup
    free(arr);
    free(indices);
    free_matrix(A, MATRIX_SIZE);
    free_matrix(B, MATRIX_SIZE);
    free_matrix(C, MATRIX_SIZE);
    
    return 0;
}
