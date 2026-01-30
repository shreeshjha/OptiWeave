#pragma once

// OptiWeave instrumentation prelude
// This file is automatically included before transformed source code

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// Performance optimization macros
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

#ifdef OPTIWEAVE_ENABLE_STATS
#include <optiweave/runtime/statistics.hpp>
#endif

#ifdef OPTIWEAVE_ENABLE_TIMING
#include <optiweave/runtime/timing.hpp>
#endif

#ifdef OPTIWEAVE_ENABLE_HOTSPOTS
#include <optiweave/runtime/hotspot_tracker.hpp>
#endif

#ifdef OPTIWEAVE_ENABLE_CACHE_PROFILE
#include <optiweave/runtime/cache_profiler.hpp>
#endif

// Forward declarations for instrumentation functions
extern "C" {
void __optiweave_log_access(const char *operation, const void *ptr,
                            std::size_t index, const char *file, int line);
void __optiweave_log_operation(const char *operation, const char *lhs_type,
                               const char *rhs_type, const char *file,
                               int line);
}

namespace optiweave {

/**
 * @brief Configuration for runtime instrumentation
 */
struct InstrumentationConfig {
  bool log_array_accesses = false;
  bool log_arithmetic_ops = false;
  bool log_to_stderr = false;
  bool log_to_file = false;
  std::string log_file_path = "optiweave.log";
  bool include_timestamps = true;
  bool include_location = true;
};

/**
 * @brief Global configuration instance
 */
extern InstrumentationConfig g_config;

/**
 * @brief SFINAE helper to detect if a type has operator[] overloaded
 */
template <typename T> struct has_subscript_overload {
private:
  template <typename U>
  static auto test(const U *u) -> decltype(u->operator[](0), std::true_type{});

  template <typename U>
  static auto test(const U &u) -> decltype(u.operator[](0), std::true_type{});

  static std::false_type test(...);

public:
  static constexpr bool value = decltype(test(std::declval<T>()))::value;
};

/**
 * @brief Helper to detect arithmetic operation overloads
 */
template <typename T, typename U> struct has_arithmetic_overload {
private:
  template <typename V, typename W>
  static auto test_add(const V &v, const W &w)
      -> decltype(v + w, std::true_type{});
  static std::false_type test_add(...);

  template <typename V, typename W>
  static auto test_sub(const V &v, const W &w)
      -> decltype(v - w, std::true_type{});
  static std::false_type test_sub(...);

  template <typename V, typename W>
  static auto test_mul(const V &v, const W &w)
      -> decltype(v * w, std::true_type{});
  static std::false_type test_mul(...);

  template <typename V, typename W>
  static auto test_div(const V &v, const W &w)
      -> decltype(v / w, std::true_type{});
  static std::false_type test_div(...);

public:
  static constexpr bool has_add =
      decltype(test_add(std::declval<T>(), std::declval<U>()))::value;
  static constexpr bool has_sub =
      decltype(test_sub(std::declval<T>(), std::declval<U>()))::value;
  static constexpr bool has_mul =
      decltype(test_mul(std::declval<T>(), std::declval<U>()))::value;
  static constexpr bool has_div =
      decltype(test_div(std::declval<T>(), std::declval<U>()))::value;
};

/**
 * @brief Primary template for array subscript instrumentation
 */
template <typename ArrayType> struct __primop_subscript {};

/**
 * @brief Specialization for C-style arrays
 */
template <typename Element, std::size_t Size>
struct __primop_subscript<Element[Size]> {
  using element_type = Element;
  using size_type = std::size_t;

  constexpr element_type &operator()(Element (&arr)[Size],
                                      size_type index) const {
// Bounds checking in debug mode
#ifdef OPTIWEAVE_DEBUG
    if (index >= Size) {
      std::cerr << "OptiWeave: Array bounds violation! Index " << index
                << " >= Size " << Size << " at " << __FILE__ << ":" << __LINE__
                << std::endl;
    }
#endif

    return arr[index];
  }
};

/**
 * @brief Specialization for pointer types
 */
template <typename Element> struct __primop_subscript<Element *> {
  using element_type = Element;
  using size_type = std::size_t;

  constexpr element_type &operator()(Element *ptr, size_type index) const {
#ifdef OPTIWEAVE_DEBUG
    if (ptr == nullptr) {
      std::cerr << "OptiWeave: Null pointer dereference at " << __FILE__ << ":"
                << __LINE__ << std::endl;
    }
#endif

    return ptr[index];
  }
};

/**
 * @brief Template for handling potentially overloaded subscript operators
 */
template <typename Subscripted, bool HasOverload>
struct __maybe_primop_subscript {
  // Default case: use the overloaded operator
  // Note: We don't use Subscripted&&/IndexType&& here because they won't work with const objects
  // Instead, use the actual forwarded types from the caller
  template <typename ArrayType, typename IndexType>
  constexpr auto operator()(ArrayType &&obj, IndexType &&index) const
      -> decltype(std::forward<ArrayType>(obj)[std::forward<IndexType>(index)]) {

    if (g_config.log_array_accesses) {
      __optiweave_log_access("overloaded_subscript", &obj,
                             static_cast<std::size_t>(index), __FILE__,
                             __LINE__);
    }

    return std::forward<ArrayType>(obj)[std::forward<IndexType>(index)];
  }
};

/**
 * @brief Specialization for types without overloaded subscript
 */
template <typename Subscripted>
struct __maybe_primop_subscript<Subscripted, false>
    : __primop_subscript<Subscripted> {};

/**
 * @brief Arithmetic operation instrumentation templates
 * Optimized for minimal overhead in the common case (no logging)
 */
template <typename LHS, typename RHS> struct __primop_add {
  OPTIWEAVE_FORCE_INLINE
#ifndef OPTIWEAVE_ENABLE_TIMING
  constexpr
#endif
  auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs + rhs) {
#ifdef OPTIWEAVE_ENABLE_TIMING
    timing::ScopedOperationTimer timer(timing::g_timing_stats.addition);
#endif

#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_addition();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("add", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs + rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_sub {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs - rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_subtraction();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("sub", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs - rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_mul {
  OPTIWEAVE_FORCE_INLINE
#ifndef OPTIWEAVE_ENABLE_TIMING
  constexpr
#endif
  auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs * rhs) {
#ifdef OPTIWEAVE_ENABLE_TIMING
    timing::ScopedOperationTimer timer(timing::g_timing_stats.multiplication);
#endif

#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_multiplication();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("mul", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs * rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_div {
  OPTIWEAVE_FORCE_INLINE
#ifndef OPTIWEAVE_ENABLE_TIMING
  constexpr
#endif
  auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs / rhs) {
#ifdef OPTIWEAVE_ENABLE_TIMING
    timing::ScopedOperationTimer timer(timing::g_timing_stats.division);
#endif

#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_division();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("div", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

#ifdef OPTIWEAVE_DEBUG
    if constexpr (std::is_arithmetic_v<RHS>) {
      if (rhs == RHS{}) {
        std::cerr << "OptiWeave: Division by zero at " << __FILE__ << ":"
                  << __LINE__ << std::endl;
      }
    }
#endif

    return lhs / rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_rem {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs % rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_modulo();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("rem", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

#ifdef OPTIWEAVE_DEBUG
    if constexpr (std::is_arithmetic_v<RHS>) {
      if (rhs == RHS{}) {
        std::cerr << "OptiWeave: Modulo by zero at " << __FILE__ << ":"
                  << __LINE__ << std::endl;
      }
    }
#endif

    return lhs % rhs;
  }
};

/**
 * @brief Comparison operation instrumentation templates
 * Optimized for minimal overhead
 */
template <typename LHS, typename RHS> struct __primop_eq {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs == rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_equal();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("eq", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs == rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_ne {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs != rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_not_equal();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("ne", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs != rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_lt {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs < rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_less_than();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("lt", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs < rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_gt {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs > rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_greater_than();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("gt", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs > rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_le {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs <= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_less_equal();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("le", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs <= rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_ge {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs >= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_greater_equal();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("ge", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs >= rhs;
  }
};

/**
 * @brief Assignment operation instrumentation templates
 * Optimized for minimal overhead
 */
template <typename LHS, typename RHS> struct __primop_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs = rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_assignment();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs = rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_add_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs += rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_add_assign();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("add_assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs += rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_sub_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs -= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_sub_assign();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("sub_assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs -= rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_mul_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs *= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_mul_assign();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("mul_assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

    return lhs *= rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_div_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs /= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_div_assign();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("div_assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

#ifdef OPTIWEAVE_DEBUG
    if constexpr (std::is_arithmetic_v<RHS>) {
      if (rhs == RHS{}) {
        std::cerr << "OptiWeave: Division by zero in /= at " << __FILE__ << ":"
                  << __LINE__ << std::endl;
      }
    }
#endif

    return lhs /= rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_mod_assign {
  OPTIWEAVE_FORCE_INLINE constexpr auto operator()(LHS &lhs, const RHS &rhs) const
      -> decltype(lhs %= rhs) {
#ifdef OPTIWEAVE_ENABLE_STATS
    statistics::increment_mod_assign();
#endif

#ifdef OPTIWEAVE_ENABLE_LOGGING
    if (OPTIWEAVE_UNLIKELY(g_config.log_arithmetic_ops)) {
      __optiweave_log_operation("mod_assign", "lhs", "rhs", __FILE__, __LINE__);
    }
#endif

#ifdef OPTIWEAVE_DEBUG
    if constexpr (std::is_arithmetic_v<RHS>) {
      if (rhs == RHS{}) {
        std::cerr << "OptiWeave: Modulo by zero in %= at " << __FILE__ << ":"
                  << __LINE__ << std::endl;
      }
    }
#endif

    return lhs %= rhs;
  }
};

/**
 * @brief Template for handling potentially overloaded arithmetic operators
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_add {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs + rhs) {

    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_add", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }

    return lhs + rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_add<LHS, RHS, false> : __primop_add<LHS, RHS> {};

/**
 * @brief SFINAE-aware subtraction template
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_sub {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs - rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_sub", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs - rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_sub<LHS, RHS, false> : __primop_sub<LHS, RHS> {};

/**
 * @brief SFINAE-aware multiplication template
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_mul {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs * rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_mul", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs * rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_mul<LHS, RHS, false> : __primop_mul<LHS, RHS> {};

/**
 * @brief SFINAE-aware division template
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_div {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs / rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_div", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs / rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_div<LHS, RHS, false> : __primop_div<LHS, RHS> {};

/**
 * @brief SFINAE-aware modulo/remainder template
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_rem {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs % rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_rem", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs % rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_rem<LHS, RHS, false> : __primop_rem<LHS, RHS> {};

/**
 * @brief SFINAE-aware comparison templates
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_eq {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs == rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_eq", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs == rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_eq<LHS, RHS, false> : __primop_eq<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_ne {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs != rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_ne", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs != rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_ne<LHS, RHS, false> : __primop_ne<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_lt {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs < rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_lt", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs < rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_lt<LHS, RHS, false> : __primop_lt<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_gt {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs > rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_gt", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs > rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_gt<LHS, RHS, false> : __primop_gt<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_le {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs <= rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_le", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs <= rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_le<LHS, RHS, false> : __primop_le<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_ge {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs >= rhs) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_ge", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs >= rhs;
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_ge<LHS, RHS, false> : __primop_ge<LHS, RHS> {};

/**
 * @brief SFINAE-aware assignment templates
 */
template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs = std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs = std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_assign<LHS, RHS, false> : __primop_assign<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_add_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs += std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_add_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs += std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_add_assign<LHS, RHS, false> : __primop_add_assign<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_sub_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs -= std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_sub_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs -= std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_sub_assign<LHS, RHS, false> : __primop_sub_assign<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_mul_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs *= std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_mul_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs *= std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_mul_assign<LHS, RHS, false> : __primop_mul_assign<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_div_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs /= std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_div_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs /= std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_div_assign<LHS, RHS, false> : __primop_div_assign<LHS, RHS> {};

template <typename LHS, typename RHS, bool HasOverload>
struct __maybe_primop_mod_assign {
  auto operator()(LHS &lhs, RHS &&rhs) const -> decltype(lhs %= std::forward<RHS>(rhs)) {
    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("overloaded_mod_assign", typeid(LHS).name(),
                                typeid(RHS).name(), __FILE__, __LINE__);
    }
    return lhs %= std::forward<RHS>(rhs);
  }
};

template <typename LHS, typename RHS>
struct __maybe_primop_mod_assign<LHS, RHS, false> : __primop_mod_assign<LHS, RHS> {};

// Similar patterns for other arithmetic operations...

/**
 * @brief Performance timing utilities
 */
class ScopedTimer {
private:
  std::chrono::high_resolution_clock::time_point start_;
  std::string operation_name_;

public:
  explicit ScopedTimer(const std::string &operation)
      : start_(std::chrono::high_resolution_clock::now()),
        operation_name_(operation) {}

  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_);

    if (g_config.log_to_stderr) {
      std::cerr << "OptiWeave: " << operation_name_ << " took "
                << duration.count() << " microseconds" << std::endl;
    }
  }
};

/**
 * @brief Compact helper wrapper functions for evaluation-safe transformations
 */

// Array subscript helper - internal version that accepts source location
template <typename Array, typename Index>
inline decltype(auto) __ow_subscript_impl(Array&& arr, Index&& idx, const char* file, int line, const char* func) {
  using DecayedArray = std::decay_t<Array>;
  using IndexType = std::decay_t<Index>;

#if defined(OPTIWEAVE_ENABLE_HOTSPOTS) || defined(OPTIWEAVE_ENABLE_TIMING)
  timing::OperationTimer hotspot_timer;
#endif

#ifdef OPTIWEAVE_ENABLE_STATS
  statistics::increment_array_subscript();
#endif

  if (g_config.log_array_accesses) {
    __optiweave_log_access("array_subscript", &arr, static_cast<size_t>(idx), file, line);
  }

  // Use __maybe_primop_subscript to handle both raw arrays and types with overloaded operator[]
  constexpr bool has_overload = has_subscript_overload<DecayedArray>::value;
  auto& result = __maybe_primop_subscript<DecayedArray, has_overload>()(std::forward<Array>(arr), std::forward<Index>(idx));

#if defined(OPTIWEAVE_ENABLE_HOTSPOTS) || defined(OPTIWEAVE_ENABLE_TIMING)
  uint64_t duration_ns = hotspot_timer.elapsed_ns();
#ifdef OPTIWEAVE_ENABLE_TIMING
  timing::g_timing_stats.array_subscript.record(duration_ns);
#endif
#ifdef OPTIWEAVE_ENABLE_HOTSPOTS
  hotspots::g_hotspot_tracker.record_operation(
      "array_subscript",
      hotspots::SourceLocation(file, line, func),
      duration_ns);
#endif
#endif

#ifdef OPTIWEAVE_ENABLE_CACHE_PROFILE
  runtime::get_cache_profiler().record_cache_stats(file, line, func);
#endif

  return result;
}

} // namespace optiweave

// Compatibility function for assignment transformations (used by comma operator syntax)
inline void __optiweave_record_subscript(const char* file, int line, const char* func) {
#ifdef OPTIWEAVE_ENABLE_STATS
  optiweave::statistics::increment_array_subscript();
#endif
#ifdef OPTIWEAVE_ENABLE_HOTSPOTS
  optiweave::hotspots::record_subscript(file, line, func);
#endif
}

// Macro version that captures source location at call site - MUST be outside namespace
#define ow_subscript(arr, idx) \
  optiweave::__ow_subscript_impl((arr), (idx), __FILE__, __LINE__, __FUNCTION__)

namespace optiweave {

// Function version for backward compatibility (captures template location)
template <typename Array, typename Index>
inline decltype(auto) ow_subscript_func(Array&& arr, Index&& idx) {
  using DecayedArray = std::decay_t<Array>;
  return __primop_subscript<DecayedArray>()(std::forward<Array>(arr), std::forward<Index>(idx));
}

// Arithmetic operation helpers
template <typename LHS, typename RHS>
inline auto ow_add(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_add<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_sub(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_sub<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_mul(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_mul<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_div(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_div<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_rem(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_rem<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

// Comparison operation helpers
template <typename LHS, typename RHS>
inline auto ow_eq(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_eq<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_ne(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_ne<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_lt(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_lt<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_gt(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_gt<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_le(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_le<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_ge(LHS&& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_ge<std::decay_t<LHS>, std::decay_t<RHS>>()(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

// Assignment operation helpers
template <typename LHS, typename RHS>
inline auto ow_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_add_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_add_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_sub_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_sub_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_mul_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_mul_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_div_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_div_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
inline auto ow_mod_assign(LHS& lhs, RHS&& rhs) -> decltype(auto) {
  return __primop_mod_assign<std::decay_t<LHS>, std::decay_t<RHS>>()(lhs, std::forward<RHS>(rhs));
}

} // namespace optiweave

/**
 * @brief Convenience macros for instrumentation
 */
#define OPTIWEAVE_INSTRUMENT_SCOPE(name)                                       \
  optiweave::ScopedTimer __optiweave_timer(name)

#define OPTIWEAVE_LOG_ACCESS(ptr, index)                                       \
  do {                                                                         \
    if (optiweave::g_config.log_array_accesses) {                              \
      __optiweave_log_access("manual", ptr, index, __FILE__, __LINE__);        \
    }                                                                          \
  } while (0)

#ifdef OPTIWEAVE_ENABLE_HOTSPOTS
// Print hotspot report manually (call before main() returns)
#define OPTIWEAVE_PRINT_HOTSPOTS(n) \
  optiweave::hotspots::print_report(n)
#else
#define OPTIWEAVE_PRINT_HOTSPOTS(n) ((void)0)
#endif

// Alias the old names for backward compatibility
#define __has_subscript_overload optiweave::has_subscript_overload
