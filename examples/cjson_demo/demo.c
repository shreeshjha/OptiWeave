/*
 * OptiWeave + cJSON Demo
 * Simple program to test instrumented cJSON
 */

#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

int main() {
    printf("OptiWeave cJSON Demo - Instrumented Build\n");
    printf("==========================================\n\n");

    // Test 1: Simple JSON parsing
    const char *json1 = "{\"name\":\"OptiWeave\",\"version\":1,\"active\":true}";
    printf("Test 1: Parsing simple JSON\n");
    printf("Input: %s\n", json1);

    cJSON *obj1 = cJSON_Parse(json1);
    if (obj1) {
        char *output = cJSON_Print(obj1);
        printf("Parsed: %s\n\n", output);
        free(output);
        cJSON_Delete(obj1);
    }

    // Test 2: Array parsing (triggers array subscript instrumentation)
    const char *json2 = "[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]";
    printf("Test 2: Parsing array (20 elements)\n");
    printf("Input: %s\n", json2);

    cJSON *arr = cJSON_Parse(json2);
    if (arr) {
        int size = cJSON_GetArraySize(arr);
        printf("Array size: %d\n", size);

        int sum = 0;
        for (int i = 0; i < size; i++) {
            cJSON *item = cJSON_GetArrayItem(arr, i);
            sum += (int)cJSON_GetNumberValue(item);
        }
        printf("Sum of elements: %d\n\n", sum);
        cJSON_Delete(arr);
    }

    // Test 3: Nested object
    const char *json3 = "{\"user\":{\"name\":\"John\",\"email\":\"john@example.com\"},\"scores\":[95,87,92]}";
    printf("Test 3: Nested object\n");
    printf("Input: %s\n", json3);

    cJSON *obj3 = cJSON_Parse(json3);
    if (obj3) {
        cJSON *user = cJSON_GetObjectItem(obj3, "user");
        cJSON *scores = cJSON_GetObjectItem(obj3, "scores");

        if (user) {
            cJSON *name = cJSON_GetObjectItem(user, "name");
            printf("User name: %s\n", cJSON_GetStringValue(name));
        }

        if (scores) {
            int count = cJSON_GetArraySize(scores);
            printf("Score count: %d\n", count);
        }

        cJSON_Delete(obj3);
    }

    printf("\n==========================================\n");
    printf("Demo completed successfully!\n");
    printf("OptiWeave tracked array subscripts during execution.\n");

    // Print runtime statistics
    extern void optiweave_print_stats(void);
    optiweave_print_stats();

    return 0;
}
