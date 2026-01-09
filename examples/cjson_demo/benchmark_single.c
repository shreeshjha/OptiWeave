/*
 * Simple benchmark for cJSON string parsing
 * Can be compiled with either baseline or optimized version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "cJSON.h"

// Complex JSON with lots of string parsing
static const char *test_json =
"{\n"
"  \"name\": \"John Smith\",\n"
"  \"email\": \"john.smith@example.com\",\n"
"  \"address\": \"123 Main Street, Apartment 4B, Springfield, IL 62701\",\n"
"  \"phone\": \"+1-555-123-4567\",\n"
"  \"description\": \"Software engineer with expertise in C/C++ development, systems programming, and performance optimization. Passionate about building efficient, maintainable code.\",\n"
"  \"skills\": [\n"
"    \"C Programming\",\n"
"    \"C++ Programming\",\n"
"    \"Performance Optimization\",\n"
"    \"Systems Programming\",\n"
"    \"Algorithm Design\"\n"
"  ],\n"
"  \"projects\": [\n"
"    {\n"
"      \"title\": \"High-Performance JSON Parser\",\n"
"      \"description\": \"Developed a fast, memory-efficient JSON parsing library with zero-copy string handling and optimized array subscript operations.\",\n"
"      \"technologies\": \"C, SIMD, Memory Profiling\",\n"
"      \"url\": \"https://github.com/example/json-parser\"\n"
"    },\n"
"    {\n"
"      \"title\": \"Real-Time Data Processing Pipeline\",\n"
"      \"description\": \"Built a multi-threaded data processing system capable of handling millions of events per second with sub-millisecond latency.\",\n"
"      \"technologies\": \"C++, Lock-Free Data Structures, Performance Tuning\",\n"
"      \"url\": \"https://github.com/example/data-pipeline\"\n"
"    }\n"
"  ],\n"
"  \"bio\": \"Experienced software engineer specializing in high-performance systems. Strong background in C/C++ with a focus on optimization, profiling, and delivering robust solutions. Enjoys solving complex technical challenges and mentoring junior developers.\",\n"
"  \"website\": \"https://www.johnsmith-dev.com\",\n"
"  \"linkedin\": \"https://www.linkedin.com/in/johnsmith-engineer\",\n"
"  \"github\": \"https://github.com/johnsmith-dev\"\n"
"}\n";

// Measure execution time in nanoseconds
static long long measure_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(int argc, char **argv) {
    int iterations = 10000;

    if (argc > 1) {
        iterations = atoi(argv[1]);
    }

    printf("cJSON String Parsing Benchmark\n");
    printf("==============================\n");
    printf("JSON size: %zu bytes\n", strlen(test_json));
    printf("Iterations: %d\n\n", iterations);

    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < 100; i++) {
        cJSON *json = cJSON_Parse(test_json);
        cJSON_Delete(json);
    }
    printf("Ready.\n\n");

    // Benchmark
    printf("Running benchmark...\n");
    long long total_time = 0;
    long long min_time = LLONG_MAX;
    long long max_time = 0;

    for (int i = 0; i < iterations; i++) {
        long long start = measure_time_ns();
        cJSON *json = cJSON_Parse(test_json);
        long long end = measure_time_ns();

        if (json == NULL) {
            fprintf(stderr, "Parse failed at iteration %d\n", i);
            return 1;
        }

        long long elapsed = end - start;
        total_time += elapsed;
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;

        cJSON_Delete(json);
    }

    printf("\nResults:\n");
    printf("--------\n");
    printf("Total time:   %10.2f ms\n", (double)total_time / 1000000.0);
    printf("Average time: %10.2f μs per iteration\n", (double)total_time / iterations / 1000.0);
    printf("Min time:     %10.2f μs\n", (double)min_time / 1000.0);
    printf("Max time:     %10.2f μs\n", (double)max_time / 1000.0);
    printf("\n");

    return 0;
}
