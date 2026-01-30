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

// Atomic counters for each operation type (thread-safe, used for final aggregation)
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

// Thread-local counters (non-atomic, fast path - no contention)
// Each thread accumulates counts locally, then flushes to global atomics on finalize
struct ThreadLocalCounters {
  // Array operations
  uint64_t array_subscript = 0;

  // Arithmetic operations
  uint64_t addition = 0;
  uint64_t subtraction = 0;
  uint64_t multiplication = 0;
  uint64_t division = 0;
  uint64_t modulo = 0;

  // Assignment operations
  uint64_t assignment = 0;
  uint64_t add_assign = 0;
  uint64_t sub_assign = 0;
  uint64_t mul_assign = 0;
  uint64_t div_assign = 0;
  uint64_t mod_assign = 0;

  // Comparison operations
  uint64_t equal = 0;
  uint64_t not_equal = 0;
  uint64_t less_than = 0;
  uint64_t greater_than = 0;
  uint64_t less_equal = 0;
  uint64_t greater_equal = 0;
};

// Global counter instance (for final aggregation)
extern OperationCounters g_counters;

// Thread-local counter instance (fast path)
extern thread_local ThreadLocalCounters tl_counters;

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

// Flush thread-local counters to global atomics
// Call this before reading global counters (e.g., in finalize)
void flush_thread_local_counters();

// Helper functions for incrementing counters (force-inlined for performance)
// Uses thread-local counters to avoid atomic contention in hot path
OPTIWEAVE_FORCE_INLINE void increment_array_subscript() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.array_subscript;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_addition() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.addition;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_subtraction() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.subtraction;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_multiplication() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.multiplication;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_division() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.division;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_modulo() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.modulo;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_assignment() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.assignment;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_add_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.add_assign;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_sub_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.sub_assign;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_mul_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.mul_assign;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_div_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.div_assign;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_mod_assign() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.mod_assign;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.equal;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_not_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.not_equal;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_less_than() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.less_than;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_greater_than() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.greater_than;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_less_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.less_equal;
  }
}

OPTIWEAVE_FORCE_INLINE void increment_greater_equal() {
  if (OPTIWEAVE_LIKELY(g_stats_enabled)) {
    ++tl_counters.greater_equal;
  }
}

} // namespace statistics
} // namespace optiweave
