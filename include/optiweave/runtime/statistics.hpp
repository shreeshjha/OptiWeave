#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

// Force inline macro for hot paths
#if defined(__GNUC__) || defined(__clang__)
  #define OPTIWEAVE_FORCE_INLINE __attribute__((always_inline)) inline
  #define OPTIWEAVE_LIKELY(x) __builtin_expect(!!(x), 1)
  #define OPTIWEAVE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
  #define OPTIWEAVE_FORCE_INLINE __forceinline
  #define OPTIWEAVE_LIKELY(x) (x)
  #define OPTIWEAVE_UNLIKELY(x) (x)
#else
  #define OPTIWEAVE_FORCE_INLINE inline
  #define OPTIWEAVE_LIKELY(x) (x)
  #define OPTIWEAVE_UNLIKELY(x) (x)
#endif

namespace optiweave {
namespace statistics {

// Atomic counters for each operation type (thread-safe)
struct OperationCounters {
  // Array operations
  std::atomic<uint64_t> array_subscript{0};

  // Arithmetic operations
  std::atomic<uint64_t> addition{0};
  std::atomic<uint64_t> subtraction{0};
  std::atomic<uint64_t> multiplication{0};
  std::atomic<uint64_t> division{0};
  std::atomic<uint64_t> modulo{0};

  // Assignment operations
  std::atomic<uint64_t> assignment{0};
  std::atomic<uint64_t> add_assign{0};
  std::atomic<uint64_t> sub_assign{0};
  std::atomic<uint64_t> mul_assign{0};
  std::atomic<uint64_t> div_assign{0};
  std::atomic<uint64_t> mod_assign{0};

  // Comparison operations
  std::atomic<uint64_t> equal{0};
  std::atomic<uint64_t> not_equal{0};
  std::atomic<uint64_t> less_than{0};
  std::atomic<uint64_t> greater_than{0};
  std::atomic<uint64_t> less_equal{0};
  std::atomic<uint64_t> greater_equal{0};

  // Bitwise operations (for future)
  std::atomic<uint64_t> bitwise_and{0};
  std::atomic<uint64_t> bitwise_or{0};
  std::atomic<uint64_t> bitwise_xor{0};
  std::atomic<uint64_t> bitwise_not{0};
  std::atomic<uint64_t> left_shift{0};
  std::atomic<uint64_t> right_shift{0};

  // Logical operations (for future)
  std::atomic<uint64_t> logical_and{0};
  std::atomic<uint64_t> logical_or{0};
  std::atomic<uint64_t> logical_not{0};
};

// Global counter instance
extern OperationCounters g_counters;

// Timing information
extern std::chrono::high_resolution_clock::time_point g_start_time;

// Control flags (set via environment or API)
extern bool g_stats_enabled;
extern std::string g_stats_csv_path;
extern std::string g_stats_json_path;

// Initialize statistics system (call at program start)
void initialize();

// Finalize and print statistics (call at program end or via atexit)
void finalize();

// Print statistics to stdout
void print_statistics();

// Export to CSV file
void export_csv(const std::string& filename);

// Export to JSON file
void export_json(const std::string& filename);

// Get total operation count
uint64_t get_total_operations();

// Get elapsed time in seconds
double get_elapsed_seconds();

// Helper functions for incrementing counters (force-inlined for performance)
OPTIWEAVE_FORCE_INLINE void increment_array_subscript() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.array_subscript.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_addition() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.addition.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_subtraction() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.subtraction.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_multiplication() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.multiplication.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_division() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.division.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_modulo() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.modulo.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_assignment() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.assignment.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_add_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.add_assign.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_sub_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.sub_assign.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_mul_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.mul_assign.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_div_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.div_assign.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_mod_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.mod_assign.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.equal.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_not_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.not_equal.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_less_than() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.less_than.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_greater_than() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.greater_than.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_less_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.less_equal.fetch_add(1, std::memory_order_relaxed);
  }
}

OPTIWEAVE_FORCE_INLINE void increment_greater_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    g_counters.greater_equal.fetch_add(1, std::memory_order_relaxed);
  }
}

} // namespace statistics
} // namespace optiweave
