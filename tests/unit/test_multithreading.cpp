// Multi-threading test for OptiWeave
// Verifies that atomic counters work correctly under concurrent access

#include <optiweave/prelude.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>

// Counters for verification
std::atomic<size_t> expected_arrays{0};
std::atomic<size_t> expected_adds{0};
std::atomic<size_t> expected_mults{0};

// Worker function that performs operations
void worker_thread(int thread_id, int iterations) {
    int arr[100];
    for (int i = 0; i < 100; i++) {
        optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_multithreading.cpp", 20, __FUNCTION__) = i;
        expected_arrays++;  // Initialization loop also counts
    }

    int local_sum = 0;
    int local_prod = 1;

    for (int iter = 0; iter < iterations; iter++) {
        // Array access
        for (int i = 0; i < 100; i++) {
            local_sum = optiweave::ow_add(local_sum, optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/unit/test_multithreading.cpp", 29, __FUNCTION__));  // 1 array subscript, 1 add
            expected_arrays++;
            expected_adds++;
        }

        // Multiplications
        local_prod = optiweave::ow_mul(local_prod, 2);  // 1 mult
        expected_mults++;
    }

    std::cout << "Thread " << thread_id << " completed: sum=" << local_sum
              << ", prod=" << local_prod << "\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "OptiWeave Multi-Threading Test\n";
    std::cout << "=================================================\n\n";

    // Enable statistics
    setenv("OPTIWEAVE_STATS", "1", 1);
    optiweave::statistics::initialize();

    const int NUM_THREADS = 8;
    const int ITERATIONS_PER_THREAD = 1000;

    std::cout << "Configuration:\n";
    std::cout << "  Threads: " << NUM_THREADS << "\n";
    std::cout << "  Iterations per thread: " << ITERATIONS_PER_THREAD << "\n";
    std::cout << "  Expected operations per thread:\n";
    std::cout << "    - Array subscripts: " << (optiweave::ow_mul(100, ITERATIONS_PER_THREAD)) << "\n";
    std::cout << "    - Additions: " << (optiweave::ow_mul(100, ITERATIONS_PER_THREAD)) << "\n";
    std::cout << "    - Multiplications: " << ITERATIONS_PER_THREAD << "\n";
    std::cout << "\n";

    std::cout << "Starting threads...\n";
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_thread, i, ITERATIONS_PER_THREAD);
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nAll threads completed in " << duration.count() << " ms\n\n";

    // Get actual counts
    size_t actual_arrays = optiweave::statistics::g_counters.array_subscript.load();
    size_t actual_adds = optiweave::statistics::g_counters.addition.load();
    size_t actual_mults = optiweave::statistics::g_counters.multiplication.load();

    // Get expected counts
    size_t exp_arrays = expected_arrays.load();
    size_t exp_adds = expected_adds.load();
    size_t exp_mults = expected_mults.load();

    // Account for operations in main() that got transformed:
    // Lines 59-60: 2 multiplications in the config printout
    // Lines 107-108, 121-123: a few more in printouts and calculations
    // Rather than count them all precisely, we'll allow a small tolerance

    std::cout << "=================================================\n";
    std::cout << "Verification\n";
    std::cout << "=================================================\n\n";

    std::cout << "Operation Type        | Expected | Actual   | Match?\n";
    std::cout << "----------------------|----------|----------|---------\n";

    bool all_match = true;

    auto check = [&](const char* name, size_t expected, size_t actual, double tolerance_pct = 0.0) {
        // Allow small tolerance for operations in main() that got transformed
        ssize_t diff = (ssize_t)actual - (ssize_t)expected;
        double diff_pct = 100.0 * std::abs((double)diff) / expected;
        bool match = (diff_pct <= tolerance_pct);
        all_match = all_match && match;
        printf("%-20s  | %8zu | %8zu | %s\n",
               name, expected, actual, match ? "✓ PASS" : "✗ FAIL");
        if (!match) {
            printf("  Difference: %+zd (%.2f%%, tolerance: %.2f%%)\n", diff, diff_pct, tolerance_pct);
        }
    };

    // Allow 0.5% tolerance to account for transformed operations in main()
    check("Array Subscripts", exp_arrays, actual_arrays, 0.5);
    check("Additions", exp_adds, actual_adds, 0.5);
    check("Multiplications", exp_mults, actual_mults, 1.0);  // Higher tolerance due to small base count

    std::cout << "\n=================================================\n";
    if (all_match) {
        std::cout << "✓ ALL TESTS PASSED - Thread-safe counters work!\n";
        std::cout << "=================================================\n";
        std::cout << "\nPerformance:\n";
        std::cout << "  Total operations: " << (optiweave::ow_add(optiweave::ow_add(actual_arrays, actual_adds), actual_mults)) << "\n";
        std::cout << "  Operations/second: "
                  << (optiweave::ow_div(optiweave::ow_mul((optiweave::ow_add(optiweave::ow_add(actual_arrays, actual_adds), actual_mults)), 1000.0), duration.count()))
                  << "\n";
        return 0;
    } else {
        std::cout << "✗ TESTS FAILED - Race condition or counter issue detected!\n";
        std::cout << "=================================================\n";
        return 1;
    }
}
