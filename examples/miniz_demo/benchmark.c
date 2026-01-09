#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "miniz.h"

#define TEST_DATA_SIZE (1024 * 1024)  // 1 MB
#define NUM_ITERATIONS 100

// Generate test data with some patterns for compression
void generate_test_data(unsigned char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Mix of patterns and random data for realistic compression
        if (i % 100 < 80) {
            data[i] = 'A' + (i % 26);  // Repeating pattern
        } else {
            data[i] = rand() % 256;     // Some randomness
        }
    }
}

double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(void) {
    unsigned char *source_data = NULL;
    unsigned char *compressed_data = NULL;
    unsigned char *decompressed_data = NULL;
    size_t compressed_size = 0;
    size_t decompressed_size = 0;
    double start_time, end_time;
    double total_time = 0.0;
    int success = 0;

    // Allocate buffers
    source_data = (unsigned char *)malloc(TEST_DATA_SIZE);
    compressed_data = (unsigned char *)malloc(TEST_DATA_SIZE * 2);  // Extra space for compressed data
    decompressed_data = (unsigned char *)malloc(TEST_DATA_SIZE);

    if (!source_data || !compressed_data || !decompressed_data) {
        fprintf(stderr, "Memory allocation failed\n");
        goto cleanup;
    }

    // Generate test data
    srand(42);  // Fixed seed for reproducibility
    generate_test_data(source_data, TEST_DATA_SIZE);

    printf("miniz Compression/Decompression Benchmark\n");
    printf("==========================================\n");
    printf("Test data size: %d KB\n", TEST_DATA_SIZE / 1024);
    printf("Iterations: %d\n\n", NUM_ITERATIONS);

    // Warmup
    compressed_size = TEST_DATA_SIZE * 2;
    mz_compress(compressed_data, &compressed_size, source_data, TEST_DATA_SIZE);

    // Benchmark compression and decompression
    start_time = get_time_ms();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Compress
        compressed_size = TEST_DATA_SIZE * 2;
        int comp_status = mz_compress(compressed_data, &compressed_size,
                                      source_data, TEST_DATA_SIZE);
        if (comp_status != MZ_OK) {
            fprintf(stderr, "Compression failed with status %d\n", comp_status);
            goto cleanup;
        }

        // Decompress
        decompressed_size = TEST_DATA_SIZE;
        int decomp_status = mz_uncompress(decompressed_data, &decompressed_size,
                                          compressed_data, compressed_size);
        if (decomp_status != MZ_OK) {
            fprintf(stderr, "Decompression failed with status %d\n", decomp_status);
            goto cleanup;
        }

        // Verify
        if (decompressed_size != TEST_DATA_SIZE ||
            memcmp(source_data, decompressed_data, TEST_DATA_SIZE) != 0) {
            fprintf(stderr, "Data verification failed at iteration %d\n", i);
            goto cleanup;
        }
    }

    end_time = get_time_ms();
    total_time = end_time - start_time;

    // Print results
    printf("Results:\n");
    printf("--------\n");
    printf("Total time: %.2f ms\n", total_time);
    printf("Average time per iteration: %.2f ms\n", total_time / NUM_ITERATIONS);
    printf("Compression ratio: %.2f%%\n",
           (double)compressed_size / TEST_DATA_SIZE * 100.0);
    printf("Compressed size: %zu bytes (%.2f KB)\n",
           compressed_size, compressed_size / 1024.0);
    printf("\nThroughput:\n");
    printf("  Compress + Decompress: %.2f MB/s\n",
           (TEST_DATA_SIZE * 2.0 * NUM_ITERATIONS / 1024.0 / 1024.0) / (total_time / 1000.0));

    success = 1;

cleanup:
    free(source_data);
    free(compressed_data);
    free(decompressed_data);

    return success ? 0 : 1;
}
