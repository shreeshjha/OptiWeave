#pragma once

// OptiWeave built-in declarations
// This file provides declarations for compiler built-ins that Clang might not
// recognize

#ifdef __cplusplus
extern "C" {
#endif

// Built-in function declarations that might be missing
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

// Variable argument pack built-ins
#if !__has_builtin(__builtin_va_arg_pack)
int __builtin_va_arg_pack(void);
#endif

#if !__has_builtin(__builtin_va_arg_pack_len)
int __builtin_va_arg_pack_len(void);
#endif

// Constant evaluation built-in
#if !__has_builtin(__builtin_is_constant_evaluated)
int __builtin_is_constant_evaluated(void);
#endif

// Memory built-ins
#if !__has_builtin(__builtin_memcpy)
void *__builtin_memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
#endif

#if !__has_builtin(__builtin_memset)
void *__builtin_memset(void *s, int c, __SIZE_TYPE__ n);
#endif

#if !__has_builtin(__builtin_memmove)
void *__builtin_memmove(void *dest, const void *src, __SIZE_TYPE__ n);
#endif

#if !__has_builtin(__builtin_memcmp)
int __builtin_memcmp(const void *s1, const void *s2, __SIZE_TYPE__ n);
#endif

// String built-ins
#if !__has_builtin(__builtin_strlen)
__SIZE_TYPE__ __builtin_strlen(const char *s);
#endif

#if !__has_builtin(__builtin_strcmp)
int __builtin_strcmp(const char *s1, const char *s2);
#endif

#if !__has_builtin(__builtin_strcpy)
char *__builtin_strcpy(char *dest, const char *src);
#endif

#if !__has_builtin(__builtin_strcat)
char *__builtin_strcat(char *dest, const char *src);
#endif

// Arithmetic built-ins
#if !__has_builtin(__builtin_abs)
int __builtin_abs(int x);
#endif

#if !__has_builtin(__builtin_labs)
long __builtin_labs(long x);
#endif

#if !__has_builtin(__builtin_llabs)
long long __builtin_llabs(long long x);
#endif

#if !__has_builtin(__builtin_fabs)
double __builtin_fabs(double x);
#endif

#if !__has_builtin(__builtin_fabsf)
float __builtin_fabsf(float x);
#endif

#if !__has_builtin(__builtin_fabsl)
long double __builtin_fabsl(long double x);
#endif

// Overflow checking built-ins
#if !__has_builtin(__builtin_add_overflow)
_Bool __builtin_add_overflow(unsigned long long a, unsigned long long b,
                             void *res);
#endif

#if !__has_builtin(__builtin_sub_overflow)
_Bool __builtin_sub_overflow(unsigned long long a, unsigned long long b,
                             void *res);
#endif

#if !__has_builtin(__builtin_mul_overflow)
_Bool __builtin_mul_overflow(unsigned long long a, unsigned long long b,
                             void *res);
#endif

// Bit manipulation built-ins
#if !__has_builtin(__builtin_clz)
int __builtin_clz(unsigned int x);
#endif

#if !__has_builtin(__builtin_clzl)
int __builtin_clzl(unsigned long x);
#endif

#if !__has_builtin(__builtin_clzll)
int __builtin_clzll(unsigned long long x);
#endif

#if !__has_builtin(__builtin_ctz)
int __builtin_ctz(unsigned int x);
#endif

#if !__has_builtin(__builtin_ctzl)
int __builtin_ctzl(unsigned long x);
#endif

#if !__has_builtin(__builtin_ctzll)
int __builtin_ctzll(unsigned long long x);
#endif

#if !__has_builtin(__builtin_popcount)
int __builtin_popcount(unsigned int x);
#endif

#if !__has_builtin(__builtin_popcountl)
int __builtin_popcountl(unsigned long x);
#endif

#if !__has_builtin(__builtin_popcountll)
int __builtin_popcountll(unsigned long long x);
#endif

// Byte swap built-ins
#if !__has_builtin(__builtin_bswap16)
unsigned short __builtin_bswap16(unsigned short x);
#endif

#if !__has_builtin(__builtin_bswap32)
unsigned int __builtin_bswap32(unsigned int x);
#endif

#if !__has_builtin(__builtin_bswap64)
unsigned long long __builtin_bswap64(unsigned long long x);
#endif

// Atomic built-ins
#if !__has_builtin(__builtin_atomic_load)
void __builtin_atomic_load(volatile void *ptr, void *ret, int memorder);
#endif

#if !__has_builtin(__builtin_atomic_store)
void __builtin_atomic_store(volatile void *ptr, void *val, int memorder);
#endif

#if !__has_builtin(__builtin_atomic_exchange)
void __builtin_atomic_exchange(volatile void *ptr, void *val, void *ret,
                               int memorder);
#endif

#if !__has_builtin(__builtin_atomic_compare_exchange)
_Bool __builtin_atomic_compare_exchange(volatile void *ptr, void *expected,
                                        void *desired, _Bool weak,
                                        int success_memorder,
                                        int failure_memorder);
#endif

// Synchronization built-ins
#if !__has_builtin(__builtin_synchronize)
void __builtin_synchronize(void);
#endif

#if !__has_builtin(__builtin_atomic_thread_fence)
void __builtin_atomic_thread_fence(int memorder);
#endif

#if !__has_builtin(__builtin_atomic_signal_fence)
void __builtin_atomic_signal_fence(int memorder);
#endif

// Control flow built-ins
#if !__has_builtin(__builtin_expect)
long __builtin_expect(long exp, long c);
#endif

#if !__has_builtin(__builtin_likely)
#define __builtin_likely(x) __builtin_expect(!!(x), 1)
#endif

#if !__has_builtin(__builtin_unlikely)
#define __builtin_unlikely(x) __builtin_expect(!!(x), 0)
#endif

#if !__has_builtin(__builtin_unreachable)
void __builtin_unreachable(void);
#endif

#if !__has_builtin(__builtin_trap)
void __builtin_trap(void);
#endif

// Object size built-ins
#if !__has_builtin(__builtin_object_size)
__SIZE_TYPE__ __builtin_object_size(const void *ptr, int type);
#endif

#if !__has_builtin(__builtin_dynamic_object_size)
__SIZE_TYPE__ __builtin_dynamic_object_size(const void *ptr, int type);
#endif

// Frame and return address built-ins
#if !__has_builtin(__builtin_return_address)
void *__builtin_return_address(unsigned int level);
#endif

#if !__has_builtin(__builtin_frame_address)
void *__builtin_frame_address(unsigned int level);
#endif

#if !__has_builtin(__builtin_extract_return_addr)
void *__builtin_extract_return_addr(void *addr);
#endif

// Stack built-ins
#if !__has_builtin(__builtin_alloca)
void *__builtin_alloca(__SIZE_TYPE__ size);
#endif

#if !__has_builtin(__builtin_alloca_with_align)
void *__builtin_alloca_with_align(__SIZE_TYPE__ size, __SIZE_TYPE__ align);
#endif

// Math built-ins
#if !__has_builtin(__builtin_inf)
double __builtin_inf(void);
#endif

#if !__has_builtin(__builtin_inff)
float __builtin_inff(void);
#endif

#if !__has_builtin(__builtin_infl)
long double __builtin_infl(void);
#endif

#if !__has_builtin(__builtin_nan)
double __builtin_nan(const char *str);
#endif

#if !__has_builtin(__builtin_nanf)
float __builtin_nanf(const char *str);
#endif

#if !__has_builtin(__builtin_nanl)
long double __builtin_nanl(const char *str);
#endif

#if !__has_builtin(__builtin_isnan)
int __builtin_isnan(double x);
#endif

#if !__has_builtin(__builtin_isinf)
int __builtin_isinf(double x);
#endif

#if !__has_builtin(__builtin_isfinite)
int __builtin_isfinite(double x);
#endif

#if !__has_builtin(__builtin_isnormal)
int __builtin_isnormal(double x);
#endif

#if !__has_builtin(__builtin_signbit)
int __builtin_signbit(double x);
#endif

// Math functions
#if !__has_builtin(__builtin_sqrt)
double __builtin_sqrt(double x);
#endif

#if !__has_builtin(__builtin_sqrtf)
float __builtin_sqrtf(float x);
#endif

#if !__has_builtin(__builtin_sqrtl)
long double __builtin_sqrtl(long double x);
#endif

#if !__has_builtin(__builtin_sin)
double __builtin_sin(double x);
#endif

#if !__has_builtin(__builtin_cos)
double __builtin_cos(double x);
#endif

#if !__has_builtin(__builtin_exp)
double __builtin_exp(double x);
#endif

#if !__has_builtin(__builtin_log)
double __builtin_log(double x);
#endif

#if !__has_builtin(__builtin_pow)
double __builtin_pow(double x, double y);
#endif

// Prefetch built-ins
#if !__has_builtin(__builtin_prefetch)
void __builtin_prefetch(const void *addr, int rw, int locality);
#endif

// CPU feature detection
#if !__has_builtin(__builtin_cpu_init)
void __builtin_cpu_init(void);
#endif

#if !__has_builtin(__builtin_cpu_is)
int __builtin_cpu_is(const char *cpu);
#endif

#if !__has_builtin(__builtin_cpu_supports)
int __builtin_cpu_supports(const char *feature);
#endif

// Debugging built-ins
#if !__has_builtin(__builtin_debugtrap)
void __builtin_debugtrap(void);
#endif

// Vector built-ins (common ones)
#if !__has_builtin(__builtin_convertvector)
// Note: This is a template-like built-in, declaration varies
#endif

#ifdef __cplusplus
}

// C++ specific built-ins
namespace std {
// Forward declarations for standard library types that might be used
// in generated template code

template <typename T> struct remove_reference;
template <typename T> struct remove_cv;
template <typename T> struct decay;
template <typename T> struct is_pointer;
template <typename T> struct is_array;
template <typename T> struct is_arithmetic;
template <typename T> struct is_integral;
template <typename T> struct is_floating_point;
template <typename T> struct is_same;
template <typename T> struct is_const;
template <typename T> struct is_volatile;

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;
template <typename T> using remove_cv_t = typename remove_cv<T>::type;
template <typename T> using decay_t = typename decay<T>::type;

template <typename T> inline constexpr bool is_pointer_v = is_pointer<T>::value;
template <typename T> inline constexpr bool is_array_v = is_array<T>::value;
template <typename T>
inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
template <typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;
template <typename T>
inline constexpr bool is_floating_point_v = is_floating_point<T>::value;
template <typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;
template <typename T> inline constexpr bool is_const_v = is_const<T>::value;
template <typename T>
inline constexpr bool is_volatile_v = is_volatile<T>::value;

// Size type
using size_t = decltype(sizeof(0));
} // namespace std

// Additional C++ built-ins for newer standards
#if __cplusplus >= 202002L // C++20
#if !__has_builtin(__builtin_is_constant_evaluated)
constexpr bool __builtin_is_constant_evaluated() noexcept;
#endif
#endif

#if __cplusplus >= 202302L // C++23
// C++23 specific built-ins if any
#endif

#endif // __cplusplus

// Platform-specific built-ins
#ifdef _MSC_VER
// MSVC specific built-ins
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#endif

#ifdef __GNUC__
// GCC specific built-ins
#endif

#ifdef __clang__
// Clang specific built-ins
#endif

// Cleanup
#undef __has_builtin
