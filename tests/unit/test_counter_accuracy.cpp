// Unit test for OptiWeave counter accuracy
// Verifies that instrumented operations are counted correctly

#include <optiweave/prelude.hpp>
#include <iostream>
#include <cassert>
#include <cstdlib>

// Expected counts - these should match exactly what the code does
struct ExpectedCounts {
    size_t array_subscripts = 0;
    size_t additions = 0;
    size_t multiplications = 0;
    size_t divisions = 0;
    size_t subtractions = 0;
};

// Test 1: Simple array operations
void test_array_operations(ExpectedCounts& expected) {
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int sum = 0;

    // 10 array subscripts in loop
    for (int i = 0; i < 10; i++) {
        sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_counter_accuracy.cpp", 25, __FUNCTION__);
        expected.array_subscripts++;
    }

    // 2 more array subscripts + 1 addition (arr[0] + arr[9])
    int x = optiweave::ow_add(optiweave::__ow_subscript_impl(arr, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_counter_accuracy.cpp", 30, __FUNCTION__), optiweave::__ow_subscript_impl(arr, 9, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_counter_accuracy.cpp", 30, __FUNCTION__));
    expected.array_subscripts += 2;
    expected.additions += 1;

    std::cout << "  Array operations: sum=" << sum << ", x=" << x << "\n";
}

// Test 2: Arithmetic operations
void test_arithmetic_operations(ExpectedCounts& expected) {
    int a = 5, b = 3, c = 2;

    // Additions
    int sum1 = optiweave::ow_add(a, b);  // 1
    int sum2 = optiweave::ow_add(sum1, c);  // 1
    expected.additions += 2;

    // Multiplications
    int prod1 = optiweave::ow_mul(a, b);  // 1
    int prod2 = optiweave::ow_mul(prod1, c);  // 1
    expected.multiplications += 2;

    // Divisions
    int div1 = optiweave::ow_div(sum2, c);  // 1
    expected.divisions += 1;

    // Subtractions
    int diff1 = optiweave::ow_sub(a, b);  // 1
    int diff2 = optiweave::ow_sub(diff1, c);  // 1
    expected.subtractions += 2;

    std::cout << "  Arithmetic: sum=" << sum2 << ", prod=" << prod2
              << ", div=" << div1 << ", diff=" << diff2 << "\n";
}

// Test 3: Loop with mixed operations
void test_loop_operations(ExpectedCounts& expected) {
    int result = 0;

    for (int i = 0; i < 100; i++) {
        result = optiweave::ow_add(result, optiweave::ow_mul(i, 2));  // 1 mult, 1 add per iteration
        expected.additions++;
        expected.multiplications++;
    }

    std::cout << "  Loop result: " << result << "\n";
}

// Test 4: Nested loops with array access
void test_nested_loops(ExpectedCounts& expected) {
    int matrix[5][5];
    int sum = 0;

    // Initialize matrix
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_counter_accuracy.cpp", 84, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_counter_accuracy.cpp", 84, __FUNCTION__) = optiweave::ow_mul(i, j);  // 1 mult per iteration (25 total)
            expected.multiplications++;
            // Note: matrix[i][j] becomes 2 nested subscript operations!
            expected.array_subscripts += 2;  // 25 iterations * 2 = 50
        }
    }

    // Transformed code has nested subscripts: __ow_subscript_impl(__ow_subscript_impl(matrix, i), j)

    std::cout << "  Nested loops complete\n";
}

// Helper to get environment variable as integer
int get_env_int(const char* name, int default_val) {
    const char* val = std::getenv(name);
    return val ? std::atoi(val) : default_val;
}

// Get actual counts from OptiWeave runtime
void get_actual_counts(size_t& arrays, size_t& adds, size_t& mults, size_t& divs, size_t& subs) {
    // Access the global counters directly (note: singular names)
    arrays = optiweave::statistics::g_counters.array_subscript.load();
    adds = optiweave::statistics::g_counters.addition.load();
    mults = optiweave::statistics::g_counters.multiplication.load();
    divs = optiweave::statistics::g_counters.division.load();
    subs = optiweave::statistics::g_counters.subtraction.load();
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "OptiWeave Counter Accuracy Test\n";
    std::cout << "=================================================\n\n";

    // Enable statistics
    setenv("OPTIWEAVE_STATS", "1", 1);
    optiweave::statistics::initialize();

    ExpectedCounts expected;

    std::cout << "Running tests...\n";
    std::cout << "Test 1: Array operations\n";
    test_array_operations(expected);

    std::cout << "Test 2: Arithmetic operations\n";
    test_arithmetic_operations(expected);

    std::cout << "Test 3: Loop with mixed operations\n";
    test_loop_operations(expected);

    std::cout << "Test 4: Nested loops\n";
    test_nested_loops(expected);

    std::cout << "\n=================================================\n";
    std::cout << "Verification\n";
    std::cout << "=================================================\n\n";

    // Get actual counts
    size_t actual_arrays, actual_adds, actual_mults, actual_divs, actual_subs;
    get_actual_counts(actual_arrays, actual_adds, actual_mults, actual_divs, actual_subs);

    // Display expected vs actual
    std::cout << "Operation Type        | Expected | Actual   | Match?\n";
    std::cout << "----------------------|----------|----------|---------\n";

    bool all_match = true;

    auto check = [&](const char* name, size_t expected, size_t actual) {
        bool match = (expected == actual);
        all_match = all_match && match;
        printf("%-20s  | %8zu | %8zu | %s\n",
               name, expected, actual, match ? "✓ PASS" : "✗ FAIL");
    };

    check("Array Subscripts", expected.array_subscripts, actual_arrays);
    check("Additions", expected.additions, actual_adds);
    check("Multiplications", expected.multiplications, actual_mults);
    check("Divisions", expected.divisions, actual_divs);
    check("Subtractions", expected.subtractions, actual_subs);

    std::cout << "\n=================================================\n";
    if (all_match) {
        std::cout << "✓ ALL TESTS PASSED - Counters are accurate!\n";
        std::cout << "=================================================\n";
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED - Counter mismatch detected!\n";
        std::cout << "=================================================\n";
        return 1;
    }
}
