#include <stdio.h>
#include <time.h>
#include "cJSON.h"

#define ITERATIONS 10000

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    const char *json = "[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]";
    
    double start = get_time();
    
    for (int i = 0; i < ITERATIONS; i++) {
        cJSON *arr = cJSON_Parse(json);
        if (arr) {
            int size = cJSON_GetArraySize(arr);
            for (int j = 0; j < size; j++) {
                cJSON *item = cJSON_GetArrayItem(arr, j);
                cJSON_GetNumberValue(item);
            }
            cJSON_Delete(arr);
        }
    }
    
    double end = get_time();
    double elapsed = end - start;
    
    printf("Iterations: %d\n", ITERATIONS);
    printf("Total time: %.4f seconds\n", elapsed);
    printf("Time per iteration: %.2f microseconds\n", (elapsed / ITERATIONS) * 1e6);
    
    return 0;
}
