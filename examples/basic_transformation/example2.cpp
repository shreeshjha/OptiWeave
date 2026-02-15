#include <optiweave/prelude.hpp>
/*
 * OptiWeave Comprehensive Feature Demonstration
 *
 * This example demonstrates all OptiWeave instrumentation and analysis features:
 * 1. Array subscript operations (1D, 2D, nested)
 * 2. Arithmetic operators (+, -, *, /, %)
 * 3. Assignment operators (=, +=, -=, *=, /=, %=)
 * 4. Comparison operators (<, >, <=, >=, ==, !=)
 * 5. Potential integer overflow scenarios
 * 6. Floating-point precision issues
 * 7. Memory allocations/deallocations
 * 8. Complex control flow for complexity analysis
 * 9. Function call chains for call graph generation
 */

#include <iostream>
#include <cmath>
#include <cstdlib>

// ============================================================================
// Section 1: Array Subscript Operations
// ============================================================================

void demonstrate_array_subscripts() {
    std::cout << "\n=== Array Subscript Operations ===" << std::endl;

    // 1D array access
    int arr1d[10];
    for (int i = 0; i < 10; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 30, __FUNCTION__); arr1d[i] = i * 2; });  // Write operation
    }
    int sum = optiweave::__ow_subscript_impl(arr1d, 0, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 32, __FUNCTION__) + optiweave::__ow_subscript_impl(arr1d, 5, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 32, __FUNCTION__) + optiweave::__ow_subscript_impl(arr1d, 9, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 32, __FUNCTION__);  // Read operations

    // 2D array access
    int matrix[5][5];
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 38, __FUNCTION__); matrix[i][j] = i * 5 + j; });  // Nested subscript
        }
    }
    int diagonal_sum = optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, 0, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__), 0, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__) + optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__), 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__) + optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__), 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 41, __FUNCTION__);

    // Pointer arithmetic and subscripting
    int data[] = {10, 20, 30, 40, 50};
    int* ptr = data;
    int value = optiweave::__ow_subscript_impl(ptr, 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 46, __FUNCTION__);  // Pointer subscript

    // Array of pointers
    int* ptr_array[3];
    int a = 1, b = 2, c = 3;
    ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 51, __FUNCTION__); ptr_array[0] = &a; });
    ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 52, __FUNCTION__); ptr_array[1] = &b; });
    ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 53, __FUNCTION__); ptr_array[2] = &c; });
    int indirect = *optiweave::__ow_subscript_impl(ptr_array, 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 54, __FUNCTION__);  // Indirect access through array

    std::cout << "Sum: " << sum << ", Diagonal: " << diagonal_sum
              << ", Value: " << value << ", Indirect: " << indirect << std::endl;
}

// ============================================================================
// Section 2: Arithmetic Operations
// ============================================================================

int demonstrate_arithmetic_ops(int x, int y) {
    std::cout << "\n=== Arithmetic Operations ===" << std::endl;

    // Basic arithmetic
    int addition = x + y;           // Addition
    int subtraction = x - y;        // Subtraction
    int multiplication = x * y;     // Multiplication
    int division = x / (y + 1);     // Division (avoid divide by zero)
    int modulo = x % (y + 1);       // Modulo

    // Compound expressions
    int complex = (x + y) * (x - y) / 2;
    int nested = ((x * 2) + (y * 3)) - ((x / 2) + (y % 5));

    // Unary operations
    int negation = -x;
    int increment = x;
    increment++;
    int decrement = y;
    decrement--;

    std::cout << "Add: " << addition << ", Sub: " << subtraction
              << ", Mul: " << multiplication << ", Div: " << division
              << ", Mod: " << modulo << std::endl;

    return complex + nested + negation + increment + decrement;
}

// ============================================================================
// Section 3: Assignment Operations
// ============================================================================

void demonstrate_assignment_ops() {
    std::cout << "\n=== Assignment Operations ===" << std::endl;

    int value = 100;  // Simple assignment

    value += 50;      // Addition assignment
    value -= 20;      // Subtraction assignment
    value *= 2;       // Multiplication assignment
    value /= 3;       // Division assignment
    value %= 50;      // Modulo assignment

    // Chained assignments
    int a, b, c;
    a = b = c = 10;

    // Assignment with complex expressions
    int arr[5] = {1, 2, 3, 4, 5};
    arr[0] += optiweave::__ow_subscript_impl(arr, 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 113, __FUNCTION__) * optiweave::__ow_subscript_impl(arr, 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 113, __FUNCTION__);
    arr[3] -= optiweave::__ow_subscript_impl(arr, 4, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 114, __FUNCTION__);

    std::cout << "Final value: " << value << ", a=" << a
              << ", arr[0]=" << arr[0] << std::endl;
}

// ============================================================================
// Section 4: Comparison Operations
// ============================================================================

bool demonstrate_comparison_ops(int x, int y) {
    std::cout << "\n=== Comparison Operations ===" << std::endl;

    bool less_than = (x < y);           // Less than
    bool greater_than = (x > y);        // Greater than
    bool less_equal = (x <= y);         // Less than or equal
    bool greater_equal = (x >= y);      // Greater than or equal
    bool equal = (x == y);              // Equality
    bool not_equal = (x != y);          // Inequality

    // Complex conditions
    bool complex = (x > 0) && (y < 100) && (x != y);
    bool nested = ((x < y) || (x == 0)) && (y >= 10);

    // Comparisons in loops
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (i >= 5 && i <= 8) {
            count++;
        }
    }

    std::cout << "LT: " << less_than << ", GT: " << greater_than
              << ", EQ: " << equal << ", Count: " << count << std::endl;

    return less_than || greater_than || equal;
}

// ============================================================================
// Section 5: Integer Overflow Scenarios (for static analysis)
// ============================================================================

void demonstrate_overflow_scenarios() {
    std::cout << "\n=== Overflow Scenarios ===" << std::endl;

    // Potential signed overflow
    int large_value = 2147483640;  // Near INT_MAX
    int overflow_risk = large_value + 100;  // May overflow

    // Unsigned wraparound
    unsigned int u_value = 4294967290U;  // Near UINT_MAX
    unsigned int wraparound = u_value + 100;  // Will wrap

    // Mixed signedness
    int signed_val = -10;
    unsigned int unsigned_val = 100;
    unsigned int mixed = signed_val + unsigned_val;  // Problematic

    // Multiplication overflow
    int a = 50000;
    int b = 50000;
    int mul_overflow = a * b;  // Likely overflow

    // Narrowing conversion
    long long big = 10000000000LL;
    int small = static_cast<int>(big);  // Narrowing

    std::cout << "Overflow risk: " << overflow_risk
              << ", Wraparound: " << wraparound
              << ", Mixed: " << mixed << std::endl;
}

// ============================================================================
// Section 6: Floating-Point Precision Issues
// ============================================================================

void demonstrate_fp_precision() {
    std::cout << "\n=== Floating-Point Precision ===" << std::endl;

    // Equality comparison (problematic)
    double d1 = 0.1 + 0.2;
    double d2 = 0.3;
    if (d1 == d2) {  // Bad: FP equality comparison
        std::cout << "Equal (unlikely)" << std::endl;
    }

    // Catastrophic cancellation
    double large = 1e15;
    double small = 1.0;
    double result = (large + small) - large;  // Loss of precision

    // Precision loss in conversion
    float f = 1.23456789012345f;
    double d = static_cast<double>(f);

    // Accumulation error
    float sum = 0.0f;
    for (int i = 0; i < 1000; i++) {
        sum += 0.001f;  // Accumulates error
    }

    // Division by very small number
    double tiny = 1e-300;
    double division = 1.0 / tiny;  // May overflow to infinity

    std::cout << "d1-d2: " << (d1 - d2) << ", result: " << result
              << ", sum: " << sum << std::endl;
}

// ============================================================================
// Section 7: Memory Operations (for memory profiling)
// ============================================================================

void demonstrate_memory_ops() {
    std::cout << "\n=== Memory Operations ===" << std::endl;

    // Dynamic allocation
    int* heap_array = new int[100];
    for (int i = 0; i < 100; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 233, __FUNCTION__); heap_array[i] = i; });
    }

    // Dynamic 2D array
    int** matrix = new int*[10];
    for (int i = 0; i < 10; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 239, __FUNCTION__); matrix[i] = new int[10]; });
        for (int j = 0; j < 10; j++) {
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 241, __FUNCTION__); matrix[i][j] = i * 10 + j; });
        }
    }

    // Use the data
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 248, __FUNCTION__), i, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 248, __FUNCTION__);
    }

    // Cleanup
    for (int i = 0; i < 10; i++) {
        delete[] optiweave::__ow_subscript_impl(matrix, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 253, __FUNCTION__);
    }
    delete[] matrix;
    delete[] heap_array;

    std::cout << "Matrix diagonal sum: " << sum << std::endl;
}

// ============================================================================
// Section 8: Complex Control Flow (for complexity analysis)
// ============================================================================

int complex_function(int n) {
    int result = 0;

    // Nested loops (high cyclomatic complexity)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                result += i;
            } else if (i < j) {
                result += j;
            } else {
                result -= i + j;
            }

            // Nested conditionals
            if (result > 100) {
                if (result % 2 == 0) {
                    result /= 2;
                } else {
                    result *= 3;
                }
            }
        }
    }

    // Switch statement
    switch (result % 5) {
        case 0: result += 10; break;
        case 1: result += 20; break;
        case 2: result += 30; break;
        case 3: result += 40; break;
        case 4: result += 50; break;
        default: break;
    }

    return result;
}

// ============================================================================
// Section 9: Function Call Chain (for call graph)
// ============================================================================

int helper_function_a(int x) {
    return x * 2;
}

int helper_function_b(int x) {
    return x + helper_function_a(x);
}

int helper_function_c(int x) {
    return helper_function_b(x) - helper_function_a(x);
}

void demonstrate_call_graph() {
    std::cout << "\n=== Call Graph Demonstration ===" << std::endl;

    int value = 10;
    int result_a = helper_function_a(value);
    int result_b = helper_function_b(value);
    int result_c = helper_function_c(value);

    std::cout << "A: " << result_a << ", B: " << result_b
              << ", C: " << result_c << std::endl;
}

// ============================================================================
// Section 10: Data Flow Scenarios (for data flow analysis)
// ============================================================================

void demonstrate_data_flow() {
    std::cout << "\n=== Data Flow Analysis ===" << std::endl;

    // Unused variable
    int unused = 100;

    // Uninitialized variable (potential issue)
    int uninitialized;

    // Write-only variable
    int write_only = 50;
    write_only = 60;
    write_only = 70;

    // Used variable
    int used = 10;
    int result = used * 2;

    // Conditional initialization
    int conditional;
    if (result > 0) {
        conditional = result;
    }
    // conditional might be uninitialized here

    std::cout << "Result: " << result << std::endl;
}

// ============================================================================
// Section 11: Hotspot Simulation (intensive operations)
// ============================================================================

void hotspot_function() {
    std::cout << "\n=== Hotspot Simulation ===" << std::endl;

    // This function performs many operations to appear in hotspot analysis
    int data[1000];
    for (int i = 0; i < 1000; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 373, __FUNCTION__); data[i] = i; });
    }

    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += optiweave::__ow_subscript_impl(data, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 378, __FUNCTION__) * 2;
        sum -= optiweave::__ow_subscript_impl(data, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example2.cpp", 379, __FUNCTION__) / 2;
        sum %= 10000;
    }

    // Nested operations
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            int idx = (i * 10 + j) % 1000;
            data[idx] += (i + j);
            data[idx] *= 2;
            data[idx] /= 3;
        }
    }

    std::cout << "Hotspot sum: " << sum << std::endl;
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  OptiWeave Comprehensive Feature Demonstration  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════╝" << std::endl;

    // Execute all demonstrations
    demonstrate_array_subscripts();

    int arith_result = demonstrate_arithmetic_ops(42, 17);
    std::cout << "Arithmetic result: " << arith_result << std::endl;

    demonstrate_assignment_ops();

    bool comp_result = demonstrate_comparison_ops(25, 30);
    std::cout << "Comparison result: " << comp_result << std::endl;

    demonstrate_overflow_scenarios();

    demonstrate_fp_precision();

    demonstrate_memory_ops();

    int complex_result = complex_function(5);
    std::cout << "\nComplex function result: " << complex_result << std::endl;

    demonstrate_call_graph();

    demonstrate_data_flow();

    hotspot_function();

    std::cout << "\n╔══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║            All Demonstrations Complete          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════╝\n" << std::endl;

    return 0;
}
