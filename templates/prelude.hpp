#pragma once

// OptiWeave instrumentation prelude
// This file is automatically included before transformed source code

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

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
  bool log_array_accesses = true;
  bool log_arithmetic_ops = false;
  bool log_to_stderr = true;
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
    if (g_config.log_array_accesses) {
      __optiweave_log_access("array_subscript", arr, index, __FILE__, __LINE__);
    }

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
    if (g_config.log_array_accesses) {
      __optiweave_log_access("pointer_subscript", ptr, index, __FILE__,
                             __LINE__);
    }

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
  template <typename IndexType>
  constexpr auto operator()(Subscripted &&obj, IndexType &&index) const
      -> decltype(std::forward<Subscripted>(
          obj)[std::forward<IndexType>(index)]) {

    if (g_config.log_array_accesses) {
      __optiweave_log_access("overloaded_subscript", &obj,
                             static_cast<std::size_t>(index), __FILE__,
                             __LINE__);
    }

    return std::forward<Subscripted>(obj)[std::forward<IndexType>(index)];
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
 */
template <typename LHS, typename RHS> struct __primop_add {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs + rhs) {

    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("add", typeid(LHS).name(), typeid(RHS).name(),
                                __FILE__, __LINE__);
    }

    return lhs + rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_sub {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs - rhs) {

    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("sub", typeid(LHS).name(), typeid(RHS).name(),
                                __FILE__, __LINE__);
    }

    return lhs - rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_mul {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs * rhs) {

    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("mul", typeid(LHS).name(), typeid(RHS).name(),
                                __FILE__, __LINE__);
    }

    return lhs * rhs;
  }
};

template <typename LHS, typename RHS> struct __primop_div {
  constexpr auto operator()(const LHS &lhs, const RHS &rhs) const
      -> decltype(lhs / rhs) {

    if (g_config.log_arithmetic_ops) {
      __optiweave_log_operation("div", typeid(LHS).name(), typeid(RHS).name(),
                                __FILE__, __LINE__);
    }

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

// Alias the old names for backward compatibility
#define __has_subscript_overload optiweave::has_subscript_overload
