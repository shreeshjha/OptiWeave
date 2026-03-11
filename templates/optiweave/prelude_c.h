/* OptiWeave C instrumentation prelude
 * This file is automatically included before transformed C source code
 * Pure C89/C99 compatible - no C++ features
 */

#ifndef OPTIWEAVE_PRELUDE_C_H
#define OPTIWEAVE_PRELUDE_C_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef OPTIWEAVE_ENABLE_STATS
#include <optiweave/runtime/statistics_c.h>
#endif

#ifdef OPTIWEAVE_ENABLE_TIMING
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for instrumentation functions */
void __optiweave_log_access(const char *operation, const void *ptr,
                            size_t index, const char *file, int line);
void __optiweave_log_operation(const char *operation, const char *lhs_type,
                               const char *rhs_type, const char *file,
                               int line);
void __optiweave_record_subscript_with_profile(const char *file, int line, const char *func);

/* Configuration structure (C-compatible) */
typedef struct {
  int log_array_accesses;
  int log_arithmetic_ops;
  int log_to_stderr;
  int log_to_file;
  const char *log_file_path;
  int include_timestamps;
  int include_location;
} OptiWeaveInstrumentationConfig;

/* Global configuration instance */
extern OptiWeaveInstrumentationConfig g_optiweave_config;

/* Helper function for subscript recording */
static inline void __optiweave_record_subscript(const char *file, int line, const char *func) {
  // Use enhanced profiling version that tracks per-function statistics
  __optiweave_record_subscript_with_profile(file, line, func);

#ifdef OPTIWEAVE_ENABLE_STATS
  optiweave_increment_array_subscript();
#endif
}

/* Array subscript instrumentation matching C++ interface
 * IMPORTANT: This must return an lvalue so assignments work: arr[i] = value
 *
 * Problem: The comma operator returns an rvalue, not an lvalue!
 *   (__optiweave_record_subscript(...), (arr)[idx]) = value  // ERROR!
 *
 * Solution: Use GNU C statement expression to execute recording then return lvalue
 *   ({ __optiweave_record_subscript(...); (arr)[idx]; })  // Works!
 *
 * For compilers without statement expressions (MSVC), fall back to side effect
 */
#if defined(__GNUC__) || defined(__clang__)
/* GNU C / Clang: Use statement expression (returns lvalue) */
#define __ow_subscript_impl(arr, idx, file, line, func) \
  ({ __optiweave_record_subscript(file, line, func); (arr)[idx]; })
#else
/* MSVC or other: No statement expressions, skip instrumentation for assignments
 * This is a limitation - subscript recording won't work for arr[i] = value
 * But will work for reads: x = arr[i]
 */
#define __ow_subscript_impl(arr, idx, file, line, func) \
  (arr)[idx]
#endif

#define ow_subscript(arr, idx) \
  (__optiweave_record_subscript(__FILE__, __LINE__, __func__), (arr)[(idx)])

/* Arithmetic operation instrumentation (C version - macro-based) */
#ifdef OPTIWEAVE_ENABLE_STATS

#define ow_add(lhs, rhs) \
  (optiweave_increment_addition(), (lhs) + (rhs))

#define ow_sub(lhs, rhs) \
  (optiweave_increment_subtraction(), (lhs) - (rhs))

#define ow_mul(lhs, rhs) \
  (optiweave_increment_multiplication(), (lhs) * (rhs))

#define ow_div(lhs, rhs) \
  (optiweave_increment_division(), (lhs) / (rhs))

#define ow_mod(lhs, rhs) \
  (optiweave_increment_modulo(), (lhs) % (rhs))
/* alias: AST visitor emits ow_rem for BO_Rem */
#define ow_rem(lhs, rhs) ow_mod(lhs, rhs)
#define ow_rem_assign(lhs, rhs) ((lhs) %= (rhs))

#else

/* No-op versions when stats disabled */
#define ow_add(lhs, rhs) ((lhs) + (rhs))
#define ow_sub(lhs, rhs) ((lhs) - (rhs))
#define ow_mul(lhs, rhs) ((lhs) * (rhs))
#define ow_div(lhs, rhs) ((lhs) / (rhs))
#define ow_mod(lhs, rhs) ((lhs) % (rhs))
#define ow_rem(lhs, rhs) ((lhs) % (rhs))
#define ow_rem_assign(lhs, rhs) ((lhs) %= (rhs))

#endif

/* Comparison operators */
#ifdef OPTIWEAVE_ENABLE_STATS

#define ow_eq(lhs, rhs) \
  (optiweave_increment_equal(), (lhs) == (rhs))

#define ow_ne(lhs, rhs) \
  (optiweave_increment_not_equal(), (lhs) != (rhs))

#define ow_lt(lhs, rhs) \
  (optiweave_increment_less_than(), (lhs) < (rhs))

#define ow_le(lhs, rhs) \
  (optiweave_increment_less_equal(), (lhs) <= (rhs))

#define ow_gt(lhs, rhs) \
  (optiweave_increment_greater_than(), (lhs) > (rhs))

#define ow_ge(lhs, rhs) \
  (optiweave_increment_greater_equal(), (lhs) >= (rhs))

#else

#define ow_eq(lhs, rhs) ((lhs) == (rhs))
#define ow_ne(lhs, rhs) ((lhs) != (rhs))
#define ow_lt(lhs, rhs) ((lhs) < (rhs))
#define ow_le(lhs, rhs) ((lhs) <= (rhs))
#define ow_gt(lhs, rhs) ((lhs) > (rhs))
#define ow_ge(lhs, rhs) ((lhs) >= (rhs))

#endif

/* Assignment operators */
#ifdef OPTIWEAVE_ENABLE_STATS

#define ow_assign(lhs, rhs) \
  (optiweave_increment_assignment(), ((lhs) = (rhs)))

#define ow_add_assign(lhs, rhs) \
  (optiweave_increment_add_assign(), ((lhs) += (rhs)))

#define ow_sub_assign(lhs, rhs) \
  (optiweave_increment_sub_assign(), ((lhs) -= (rhs)))

#define ow_mul_assign(lhs, rhs) \
  (optiweave_increment_mul_assign(), ((lhs) *= (rhs)))

#define ow_div_assign(lhs, rhs) \
  (optiweave_increment_div_assign(), ((lhs) /= (rhs)))

#else

#define ow_assign(lhs, rhs) ((lhs) = (rhs))
#define ow_add_assign(lhs, rhs) ((lhs) += (rhs))
#define ow_sub_assign(lhs, rhs) ((lhs) -= (rhs))
#define ow_mul_assign(lhs, rhs) ((lhs) *= (rhs))
#define ow_div_assign(lhs, rhs) ((lhs) /= (rhs))

#endif

/* Timing utilities for C */
#ifdef OPTIWEAVE_ENABLE_TIMING

typedef struct {
  struct timespec start;
  const char *operation_name;
} OptiWeaveScopedTimer;

static inline void optiweave_timer_start(OptiWeaveScopedTimer *timer, const char *operation) {
  timer->operation_name = operation;
  clock_gettime(CLOCK_MONOTONIC, &timer->start);
}

static inline void optiweave_timer_end(OptiWeaveScopedTimer *timer) {
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);

  uint64_t duration_ns = (end.tv_sec - timer->start.tv_sec) * 1000000000ULL +
                         (end.tv_nsec - timer->start.tv_nsec);

  if (g_optiweave_config.log_to_stderr) {
    fprintf(stderr, "OptiWeave: %s took %llu nanoseconds\n",
            timer->operation_name, (unsigned long long)duration_ns);
  }
}

#define OPTIWEAVE_INSTRUMENT_SCOPE(name) \
  OptiWeaveScopedTimer __optiweave_timer; \
  optiweave_timer_start(&__optiweave_timer, name); \
  /* Will need manual timer_end call or cleanup */

#else

#define OPTIWEAVE_INSTRUMENT_SCOPE(name) ((void)0)

#endif

/* Debug helpers */
#ifdef OPTIWEAVE_DEBUG

#define OPTIWEAVE_BOUNDS_CHECK(arr, idx, size) \
  do { \
    if ((idx) >= (size)) { \
      fprintf(stderr, "OptiWeave: Array bounds violation! Index %zu >= Size %zu at %s:%d\n", \
              (size_t)(idx), (size_t)(size), __FILE__, __LINE__); \
    } \
  } while (0)

#define OPTIWEAVE_NULL_CHECK(ptr) \
  do { \
    if ((ptr) == NULL) { \
      fprintf(stderr, "OptiWeave: Null pointer dereference at %s:%d\n", \
              __FILE__, __LINE__); \
    } \
  } while (0)

#define OPTIWEAVE_DIV_ZERO_CHECK(divisor) \
  do { \
    if ((divisor) == 0) { \
      fprintf(stderr, "OptiWeave: Division by zero at %s:%d\n", \
              __FILE__, __LINE__); \
    } \
  } while (0)

#else

#define OPTIWEAVE_BOUNDS_CHECK(arr, idx, size) ((void)0)
#define OPTIWEAVE_NULL_CHECK(ptr) ((void)0)
#define OPTIWEAVE_DIV_ZERO_CHECK(divisor) ((void)0)

#endif

/* Convenience macros */
#define OPTIWEAVE_LOG_ACCESS(ptr, index) \
  do { \
    if (g_optiweave_config.log_array_accesses) { \
      __optiweave_log_access("manual", ptr, index, __FILE__, __LINE__); \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* OPTIWEAVE_PRELUDE_C_H */
