/*
 * AES-128 Benchmark with OptiWeave Instrumentation
 * 
 * This benchmark runs 100,000 AES-128 ECB mode encryption operations
 * using NIST test vectors to validate correctness and measure performance.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "aes.h"

// OptiWeave runtime function
extern void optiweave_print_stats(void);

// NIST test vector for AES-128 ECB mode
static const uint8_t test_key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t test_plaintext[16] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
};

static const uint8_t expected_ciphertext[16] = {
    0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
    0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
};

// Print hex bytes
static void print_hex(const char* label, const uint8_t* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// Verify correctness against NIST test vector
static int verify_correctness(void) {
    uint8_t buffer[16];
    struct AES_ctx ctx;
    
    memcpy(buffer, test_plaintext, 16);
    AES_init_ctx(&ctx, test_key);
    AES_ECB_encrypt(&ctx, buffer);
    
    if (memcmp(buffer, expected_ciphertext, 16) != 0) {
        printf("ERROR: Encryption failed NIST test vector!\n");
        print_hex("Expected", expected_ciphertext, 16);
        print_hex("Got     ", buffer, 16);
        return 0;
    }
    
    printf("✓ NIST test vector passed\n");
    return 1;
}

// Benchmark function
static void run_benchmark(int iterations) {
    uint8_t buffer[16];
    struct AES_ctx ctx;
    clock_t start, end;
    double cpu_time;
    
    printf("\n=== Running AES-128 ECB Benchmark ===\n");
    printf("Iterations: %d\n", iterations);
    printf("Block size: 16 bytes\n");
    
    // Initialize
    AES_init_ctx(&ctx, test_key);
    
    // Start timing
    start = clock();
    
    // Run iterations
    for (int i = 0; i < iterations; i++) {
        memcpy(buffer, test_plaintext, 16);
        AES_ECB_encrypt(&ctx, buffer);
    }
    
    // End timing
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Calculate metrics
    double ops_per_sec = iterations / cpu_time;
    double mb_per_sec = (iterations * 16.0) / (1024.0 * 1024.0 * cpu_time);
    double us_per_op = (cpu_time * 1000000.0) / iterations;
    
    printf("\n=== Performance Results ===\n");
    printf("Total time:        %.3f seconds\n", cpu_time);
    printf("Operations/sec:    %.0f\n", ops_per_sec);
    printf("Throughput:        %.2f MB/s\n", mb_per_sec);
    printf("Time per op:       %.2f µs\n", us_per_op);
    printf("Total data:        %.2f MB\n", (iterations * 16.0) / (1024.0 * 1024.0));
}

int main(void) {
    printf("OptiWeave AES-128 Benchmark Demo\n");
    printf("=================================\n\n");
    
    // Verify correctness first
    if (!verify_correctness()) {
        return 1;
    }
    
    // Run benchmark with 100,000 iterations
    run_benchmark(100000);
    
    printf("\n=== OptiWeave Profiling Data ===\n");
    optiweave_print_stats();
    
    return 0;
}
