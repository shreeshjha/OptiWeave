/**
 * OptiWeave Static Analysis Demo
 * Demonstrates complexity analysis, call graph, and bug detection capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// FUNCTION 1: Simple - Low Complexity (Cyclomatic: 1)
// ============================================================================
int add_numbers(int a, int b) {
    return a + b;
}

// ============================================================================
// FUNCTION 2: Medium Complexity (Cyclomatic: 4)
// ============================================================================
int classify_number(int n) {
    if (n < 0) {
        return -1;  // Negative
    } else if (n == 0) {
        return 0;   // Zero
    } else if (n < 10) {
        return 1;   // Single digit
    } else {
        return 2;   // Multi-digit
    }
}

// ============================================================================
// FUNCTION 3: High Complexity (Cyclomatic: 8+)
// This function is too complex - should be refactored!
// ============================================================================
int process_data(int* data, int len, int mode) {
    int result = 0;
    
    if (data == NULL) return -1;
    if (len <= 0) return -2;
    
    switch (mode) {
        case 0:  // Sum
            for (int i = 0; i < len; i++) {
                result += data[i];  // Array access hotspot
            }
            break;
        case 1:  // Product
            result = 1;
            for (int i = 0; i < len; i++) {
                result *= data[i];
                if (result == 0) break;  // Short-circuit
            }
            break;
        case 2:  // Max
            result = data[0];
            for (int i = 1; i < len; i++) {
                if (data[i] > result) {
                    result = data[i];
                }
            }
            break;
        case 3:  // Count positive
            for (int i = 0; i < len; i++) {
                if (data[i] > 0) result++;
            }
            break;
        default:
            return -3;
    }
    
    return result;
}

// ============================================================================
// FUNCTION 4: Contains potential bugs
// OptiWeave static analysis will flag these
// ============================================================================
int vulnerable_function(int user_input, int multiplier) {
    // Bug 1: Potential signed overflow
    int index = user_input * multiplier;  // OptiWeave: overflow warning
    
    // Bug 2: Negative array index possible
    int buffer[100];
    buffer[index] = 42;  // OptiWeave: negative index warning
    
    return buffer[0];
}

// ============================================================================
// FUNCTION 5: Nested loops - O(n^2) complexity
// ============================================================================
int nested_search(int* arr, int n, int target) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (arr[i] + arr[j] == target) {
                return i * n + j;  // Found pair
            }
        }
    }
    return -1;  // Not found
}

// ============================================================================
// FUNCTION 6: Deep nesting - cognitive complexity issue
// ============================================================================
int deeply_nested(int a, int b, int c, int d) {
    int result = 0;
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                if (d > 0) {
                    result = a + b + c + d;  // Too deeply nested!
                } else {
                    result = a + b + c;
                }
            } else {
                result = a + b;
            }
        } else {
            result = a;
        }
    }
    return result;
}

// ============================================================================
// MAIN - Demonstrates the call graph
// ============================================================================
int main(int argc, char* argv[]) {
    printf("=== OptiWeave Static Analysis Demo ===\n\n");
    
    // Test simple function
    int sum = add_numbers(5, 3);
    printf("add_numbers(5, 3) = %d\n", sum);
    
    // Test classifier
    printf("classify_number(-5) = %d\n", classify_number(-5));
    printf("classify_number(0) = %d\n", classify_number(0));
    printf("classify_number(7) = %d\n", classify_number(7));
    
    // Test data processing
    int data[] = {1, 2, 3, 4, 5};
    printf("process_data (sum) = %d\n", process_data(data, 5, 0));
    printf("process_data (max) = %d\n", process_data(data, 5, 2));
    
    // Test nested search
    int arr[] = {1, 2, 3, 4, 5};
    printf("nested_search (target=5) = %d\n", nested_search(arr, 5, 5));
    
    // Test deeply nested
    printf("deeply_nested(1,2,3,4) = %d\n", deeply_nested(1, 2, 3, 4));
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
