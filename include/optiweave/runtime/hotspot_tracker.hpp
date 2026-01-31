#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Performance optimization macros
#if defined(__GNUC__) || defined(__clang__)
  #define OPTIWEAVE_HOTSPOT_FORCE_INLINE __attribute__((always_inline)) inline
  #define OPTIWEAVE_HOTSPOT_LIKELY(x) __builtin_expect(!!(x), 1)
  #define OPTIWEAVE_HOTSPOT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define OPTIWEAVE_HOTSPOT_FORCE_INLINE inline
  #define OPTIWEAVE_HOTSPOT_LIKELY(x) (x)
  #define OPTIWEAVE_HOTSPOT_UNLIKELY(x) (x)
#endif

// Fast cycle counter for x86_64 (much faster than std::chrono)
#if defined(__x86_64__) || defined(_M_X64)
  #define OPTIWEAVE_HAS_RDTSC 1
  OPTIWEAVE_HOTSPOT_FORCE_INLINE uint64_t optiweave_rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
  }
  #define OPTIWEAVE_GET_CYCLES() optiweave_rdtsc()
#else
  #define OPTIWEAVE_HAS_RDTSC 0
  #define OPTIWEAVE_GET_CYCLES() std::chrono::high_resolution_clock::now().time_since_epoch().count()
#endif

// Default sampling rate (1 in N operations will be fully timed)
#ifndef OPTIWEAVE_HOTSPOT_SAMPLE_RATE
  #define OPTIWEAVE_HOTSPOT_SAMPLE_RATE 100
#endif

namespace optiweave {
namespace hotspots {

/**
 * @brief Operation types for fast array-indexed counting
 * OPTIMIZED: Replaces string-keyed map to avoid heap allocations
 */
enum class OperationType : uint8_t {
  ARRAY_SUBSCRIPT = 0,
  ADDITION,
  SUBTRACTION,
  MULTIPLICATION,
  DIVISION,
  MODULO,
  ASSIGNMENT,
  ADD_ASSIGN,
  SUB_ASSIGN,
  MUL_ASSIGN,
  DIV_ASSIGN,
  MOD_ASSIGN,
  EQUAL,
  NOT_EQUAL,
  LESS_THAN,
  GREATER_THAN,
  LESS_EQUAL,
  GREATER_EQUAL,
  BITWISE_AND,
  BITWISE_OR,
  BITWISE_XOR,
  LEFT_SHIFT,
  RIGHT_SHIFT,
  LOGICAL_AND,
  LOGICAL_OR,
  OTHER,  // Catch-all for unknown operations
  OP_TYPE_COUNT  // Must be last
};

// Convert operation type to string for display
inline const char* operation_type_to_string(OperationType op) {
  static const char* names[] = {
    "array_subscript", "addition", "subtraction", "multiplication", "division",
    "modulo", "assignment", "add_assign", "sub_assign", "mul_assign",
    "div_assign", "mod_assign", "equal", "not_equal", "less_than",
    "greater_than", "less_equal", "greater_equal", "bitwise_and", "bitwise_or",
    "bitwise_xor", "left_shift", "right_shift", "logical_and", "logical_or", "other"
  };
  auto idx = static_cast<size_t>(op);
  return idx < static_cast<size_t>(OperationType::OP_TYPE_COUNT) ? names[idx] : "unknown";
}

// Convert string to operation type (for compatibility)
inline OperationType string_to_operation_type(const char* str) {
  if (!str || !*str) return OperationType::OTHER;
  
  // Fast prefix matching
  switch (str[0]) {
    case 'a':
      if (std::strcmp(str, "array_subscript") == 0) return OperationType::ARRAY_SUBSCRIPT;
      if (std::strcmp(str, "addition") == 0) return OperationType::ADDITION;
      if (std::strcmp(str, "assignment") == 0) return OperationType::ASSIGNMENT;
      if (std::strcmp(str, "add_assign") == 0) return OperationType::ADD_ASSIGN;
      break;
    case 's':
      if (std::strcmp(str, "subtraction") == 0) return OperationType::SUBTRACTION;
      if (std::strcmp(str, "sub_assign") == 0) return OperationType::SUB_ASSIGN;
      break;
    case 'm':
      if (std::strcmp(str, "multiplication") == 0) return OperationType::MULTIPLICATION;
      if (std::strcmp(str, "modulo") == 0) return OperationType::MODULO;
      if (std::strcmp(str, "mul_assign") == 0) return OperationType::MUL_ASSIGN;
      if (std::strcmp(str, "mod_assign") == 0) return OperationType::MOD_ASSIGN;
      break;
    case 'd':
      if (std::strcmp(str, "division") == 0) return OperationType::DIVISION;
      if (std::strcmp(str, "div_assign") == 0) return OperationType::DIV_ASSIGN;
      break;
    case 'e':
      if (std::strcmp(str, "equal") == 0) return OperationType::EQUAL;
      break;
    case 'n':
      if (std::strcmp(str, "not_equal") == 0) return OperationType::NOT_EQUAL;
      break;
    case 'l':
      if (std::strcmp(str, "less_than") == 0) return OperationType::LESS_THAN;
      if (std::strcmp(str, "less_equal") == 0) return OperationType::LESS_EQUAL;
      if (std::strcmp(str, "left_shift") == 0) return OperationType::LEFT_SHIFT;
      if (std::strcmp(str, "logical_and") == 0) return OperationType::LOGICAL_AND;
      if (std::strcmp(str, "logical_or") == 0) return OperationType::LOGICAL_OR;
      break;
    case 'g':
      if (std::strcmp(str, "greater_than") == 0) return OperationType::GREATER_THAN;
      if (std::strcmp(str, "greater_equal") == 0) return OperationType::GREATER_EQUAL;
      break;
    case 'b':
      if (std::strcmp(str, "bitwise_and") == 0) return OperationType::BITWISE_AND;
      if (std::strcmp(str, "bitwise_or") == 0) return OperationType::BITWISE_OR;
      if (std::strcmp(str, "bitwise_xor") == 0) return OperationType::BITWISE_XOR;
      break;
    case 'r':
      if (std::strcmp(str, "right_shift") == 0) return OperationType::RIGHT_SHIFT;
      break;
  }
  return OperationType::OTHER;
}

/**
 * @brief Represents a source code location (OPTIMIZED)
 * 
 * Uses const char* pointers by default to avoid heap allocations in hot path.
 * String literals in C/C++ have static storage duration, so pointers are safe.
 * Pre-computes hash for O(1) lookup performance.
 * 
 * For cases where string storage is needed (e.g., deserialization), use
 * the string constructor or set_file()/set_function() methods.
 */
struct SourceLocation {
  const char* file;
  int line;
  const char* function;
  mutable size_t hash_cache;  // Cached hash for fast lookup
  
  // Optional owned string storage for cases like deserialization
  // These are only used when strings need to be stored (not just pointed to)
  std::unique_ptr<std::string> file_storage;
  std::unique_ptr<std::string> function_storage;

  // Fast constructor for static string literals (most common case)
  OPTIWEAVE_HOTSPOT_FORCE_INLINE
  SourceLocation(const char *f = "", int l = 0, const char *fn = "")
      : file(f ? f : ""), line(l), function(fn ? fn : ""), hash_cache(0),
        file_storage(nullptr), function_storage(nullptr) {
    hash_cache = compute_hash();
  }
  
  // Constructor for std::string (copies and stores the strings)
  SourceLocation(const std::string& f, int l, const std::string& fn)
      : line(l), hash_cache(0),
        file_storage(std::make_unique<std::string>(f)),
        function_storage(std::make_unique<std::string>(fn)) {
    file = file_storage->c_str();
    function = function_storage->c_str();
    hash_cache = compute_hash_from_content();
  }
  
  // Copy constructor (deep copy if using storage)
  SourceLocation(const SourceLocation& other)
      : line(other.line), hash_cache(other.hash_cache) {
    if (other.file_storage) {
      file_storage = std::make_unique<std::string>(*other.file_storage);
      file = file_storage->c_str();
    } else {
      file = other.file;
    }
    if (other.function_storage) {
      function_storage = std::make_unique<std::string>(*other.function_storage);
      function = function_storage->c_str();
    } else {
      function = other.function;
    }
  }
  
  // Move constructor
  SourceLocation(SourceLocation&& other) noexcept
      : file(other.file), line(other.line), function(other.function),
        hash_cache(other.hash_cache),
        file_storage(std::move(other.file_storage)),
        function_storage(std::move(other.function_storage)) {
    // Update pointers if we moved storage
    if (file_storage) file = file_storage->c_str();
    if (function_storage) function = function_storage->c_str();
  }
  
  // Copy assignment
  SourceLocation& operator=(const SourceLocation& other) {
    if (this != &other) {
      line = other.line;
      hash_cache = other.hash_cache;
      if (other.file_storage) {
        file_storage = std::make_unique<std::string>(*other.file_storage);
        file = file_storage->c_str();
      } else {
        file_storage.reset();
        file = other.file;
      }
      if (other.function_storage) {
        function_storage = std::make_unique<std::string>(*other.function_storage);
        function = function_storage->c_str();
      } else {
        function_storage.reset();
        function = other.function;
      }
    }
    return *this;
  }
  
  // Move assignment
  SourceLocation& operator=(SourceLocation&& other) noexcept {
    if (this != &other) {
      file = other.file;
      line = other.line;
      function = other.function;
      hash_cache = other.hash_cache;
      file_storage = std::move(other.file_storage);
      function_storage = std::move(other.function_storage);
      if (file_storage) file = file_storage->c_str();
      if (function_storage) function = function_storage->c_str();
    }
    return *this;
  }
  
  // Setters for owned strings (for deserialization)
  void set_file(const std::string& f) {
    file_storage = std::make_unique<std::string>(f);
    file = file_storage->c_str();
    hash_cache = compute_hash_from_content();
  }
  
  void set_function(const std::string& fn) {
    function_storage = std::make_unique<std::string>(fn);
    function = function_storage->c_str();
    hash_cache = compute_hash_from_content();
  }

  OPTIWEAVE_HOTSPOT_FORCE_INLINE
  size_t compute_hash() const {
    // FNV-1a hash using pointer values (very fast for static strings)
    constexpr size_t FNV_PRIME = 1099511628211ULL;
    constexpr size_t FNV_OFFSET = 14695981039346656037ULL;
    
    size_t h = FNV_OFFSET;
    h ^= reinterpret_cast<size_t>(file);
    h *= FNV_PRIME;
    h ^= static_cast<size_t>(line);
    h *= FNV_PRIME;
    h ^= reinterpret_cast<size_t>(function);
    h *= FNV_PRIME;
    return h;
  }
  
  // Hash from content (for dynamic strings)
  size_t compute_hash_from_content() const {
    constexpr size_t FNV_PRIME = 1099511628211ULL;
    constexpr size_t FNV_OFFSET = 14695981039346656037ULL;
    
    size_t h = FNV_OFFSET;
    // Hash file string contents
    for (const char* p = file; p && *p; ++p) {
      h ^= static_cast<size_t>(*p);
      h *= FNV_PRIME;
    }
    h ^= static_cast<size_t>(line);
    h *= FNV_PRIME;
    // Hash function string contents
    for (const char* p = function; p && *p; ++p) {
      h ^= static_cast<size_t>(*p);
      h *= FNV_PRIME;
    }
    return h;
  }

  OPTIWEAVE_HOTSPOT_FORCE_INLINE
  bool operator==(const SourceLocation &other) const {
    // Fast path: compare line first (most likely to differ)
    if (line != other.line) return false;
    // Fast path: compare pointers (same string literal = same address)
    if (file == other.file && function == other.function) return true;
    // Slow path: compare string contents
    return std::strcmp(file, other.file) == 0 && 
           std::strcmp(function, other.function) == 0;
  }

  std::string to_string() const {
    std::string result = file;
    result += ":";
    result += std::to_string(line);
    if (function && function[0] != '\0') {
      result += " (";
      result += function;
      result += ")";
    }
    return result;
  }
  
  // For compatibility with code expecting std::string file/function
  std::string get_file() const { return file ? file : ""; }
  std::string get_function() const { return function ? function : ""; }
};

} // namespace hotspots
} // namespace optiweave

// Optimized hash function for SourceLocation
namespace std {
template <> struct hash<optiweave::hotspots::SourceLocation> {
  OPTIWEAVE_HOTSPOT_FORCE_INLINE
  size_t operator()(const optiweave::hotspots::SourceLocation &loc) const {
    // Use pre-computed hash - O(1)!
    return loc.hash_cache;
  }
};
} // namespace std

namespace optiweave {
namespace hotspots {

/**
 * @brief Information about a performance hotspot
 * OPTIMIZED: Uses fixed-size array instead of string-keyed map for operation counts
 */
struct HotspotInfo {
  SourceLocation location;
  uint64_t operation_count = 0;
  uint64_t total_time_ns = 0;
  
  // OPTIMIZED: Fixed-size array indexed by OperationType - no heap allocations!
  std::array<uint64_t, static_cast<size_t>(OperationType::OP_TYPE_COUNT)> operation_counts{};
  
  // Increment count for a specific operation type
  OPTIWEAVE_HOTSPOT_FORCE_INLINE void record_operation(OperationType op) {
    ++operation_count;
    ++operation_counts[static_cast<size_t>(op)];
  }
  
  // Get count for a specific operation type
  uint64_t get_operation_count(OperationType op) const {
    return operation_counts[static_cast<size_t>(op)];
  }
  
  // Compatibility: Get operation breakdown as map (for reporting/export)
  std::unordered_map<std::string, uint64_t> get_operation_breakdown() const {
    std::unordered_map<std::string, uint64_t> result;
    for (size_t i = 0; i < static_cast<size_t>(OperationType::OP_TYPE_COUNT); ++i) {
      if (operation_counts[i] > 0) {
        result[operation_type_to_string(static_cast<OperationType>(i))] = operation_counts[i];
      }
    }
    return result;
  }

  double avg_time_ns() const {
    return operation_count > 0 ? static_cast<double>(total_time_ns) / operation_count : 0.0;
  }

  double percentage_of_total(uint64_t total_runtime_ns) const {
    return total_runtime_ns > 0
               ? 100.0 * static_cast<double>(total_time_ns) / total_runtime_ns
               : 0.0;
  }
};

/**
 * @brief Thread-local hotspot buffer for lock-free fast path
 * Each thread accumulates hotspot data locally, then flushes to global tracker
 * OPTIMIZED: Pre-reserves capacity to avoid rehashing in hot path
 */
struct ThreadLocalHotspotBuffer {
  std::unordered_map<SourceLocation, HotspotInfo> hotspots;
  
  // Flush threshold - flush when buffer reaches this size
  static constexpr size_t FLUSH_THRESHOLD = 1000;
  
  // Initial capacity to avoid early rehashing
  static constexpr size_t INITIAL_CAPACITY = 256;
  
  // Track if this buffer has been registered for flushing
  bool registered = false;
  
  // Constructor pre-reserves capacity
  ThreadLocalHotspotBuffer() {
    hotspots.reserve(INITIAL_CAPACITY);
  }
};

/**
 * @brief Tracks performance hotspots by source location
 */
class HotspotTracker {
private:
  // Use pointer to avoid destruction order issues with atexit
  // Never deleted to ensure data persists until program exit
  std::unordered_map<SourceLocation, HotspotInfo>* hotspots_;
  mutable std::mutex mutex_;
  std::chrono::high_resolution_clock::time_point start_time_;
  uint64_t total_runtime_ns_ = 0;

public:
  HotspotTracker();

  /**
   * @brief Record an operation at a specific source location
   */
  void record_operation(const std::string &op_type, const SourceLocation &loc,
                        uint64_t duration_ns);

  /**
   * @brief Get top N hotspots sorted by total time
   */
  std::vector<HotspotInfo> get_top_hotspots(size_t n = 10) const;

  /**
   * @brief Get hotspots aggregated by function
   */
  std::map<std::string, HotspotInfo> get_hotspots_by_function() const;

  /**
   * @brief Get hotspots aggregated by file
   */
  std::map<std::string, HotspotInfo> get_hotspots_by_file() const;

  /**
   * @brief Get total runtime in nanoseconds
   */
  uint64_t get_total_runtime_ns() const;

  /**
   * @brief Get number of unique hotspot locations
   */
  size_t get_location_count() const;

  /**
   * @brief Print hotspot analysis to console
   */
  void print_hotspots(size_t top_n = 10) const;

  /**
   * @brief Export hotspot data to CSV
   */
  void export_csv(const std::string &filename) const;

  /**
   * @brief Export hotspot data to JSON
   */
  void export_json(const std::string &filename) const;

  /**
   * @brief Export hotspot data to flame graph format (folded stacks)
   * Compatible with flamegraph.pl and speedscope.app
   */
  void export_flamegraph(const std::string &filename) const;

  /**
   * @brief Export hotspot data to interactive HTML report
   */
  void export_html(const std::string &filename) const;

  /**
   * @brief Finalize tracking (stop timer)
   */
  void finalize();

  /**
   * @brief Merge thread-local hotspot buffer into global tracker
   * Thread-safe: uses internal mutex
   */
  void merge_thread_local_buffer(ThreadLocalHotspotBuffer& buffer);
};

// Global hotspot tracker instance
extern HotspotTracker g_hotspot_tracker;

// Thread-local hotspot buffer (fast path - no locking)
extern thread_local ThreadLocalHotspotBuffer tl_hotspot_buffer;

// Runtime flags
extern bool g_hotspots_enabled;
extern size_t g_hotspots_top_n;
extern std::string g_hotspots_csv_file;
extern std::string g_hotspots_json_file;
extern std::string g_hotspots_flamegraph_file;
extern std::string g_hotspots_html_file;

/**
 * @brief Initialize hotspot tracking
 */
void initialize();

/**
 * @brief Finalize and print hotspot statistics (internal, called by atexit)
 */
void finalize();

/**
 * @brief Flush thread-local hotspot buffer to global tracker
 * Call this before reading global hotspot data (e.g., before finalize)
 */
void flush_thread_local_hotspots();

/**
 * @brief Manually print hotspot report (call this before program exits)
 * This is the recommended way to get hotspot reports, as it avoids
 * potential corruption issues with atexit() handlers.
 */
void print_report(size_t top_n = 10);

/**
 * @brief Helper function to record array subscript operations
 * Called from instrumented code in prelude.hpp
 * Uses thread-local buffering for lock-free fast path
 * OPTIMIZED: No mutex, no heap allocations in hot path
 */
OPTIWEAVE_HOTSPOT_FORCE_INLINE
void record_subscript(const char* file, int line, const char* func) {
  // Early exit when disabled - use UNLIKELY since when profiling is active,
  // we expect to take the recording path most of the time
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(!g_hotspots_enabled)) return;
  
  SourceLocation loc(file, line, func);
  
  // Fast path: record in thread-local buffer (no locking)
  auto& info = tl_hotspot_buffer.hotspots[loc];
  info.location = loc;
  info.record_operation(OperationType::ARRAY_SUBSCRIPT);
  
  // Periodic flush to avoid unbounded memory growth
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(tl_hotspot_buffer.hotspots.size() > ThreadLocalHotspotBuffer::FLUSH_THRESHOLD)) {
    flush_thread_local_hotspots();
  }
}

/**
 * @brief Helper function to record array subscript with timing
 * OPTIMIZED: Lock-free, uses RDTSC for fast timing, thread-local buffering
 */
OPTIWEAVE_HOTSPOT_FORCE_INLINE
void record_subscript_with_timing(const char* file, int line, const char* func, uint64_t duration_ns) {
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(!g_hotspots_enabled)) return;
  
  SourceLocation loc(file, line, func);
  
  // Fast path: record in thread-local buffer (no locking)
  auto& info = tl_hotspot_buffer.hotspots[loc];
  info.location = loc;
  info.total_time_ns += duration_ns;
  info.operation_counts[static_cast<size_t>(OperationType::ARRAY_SUBSCRIPT)]++;
  info.operation_count++;
  
  // Periodic flush to avoid unbounded memory growth
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(tl_hotspot_buffer.hotspots.size() > ThreadLocalHotspotBuffer::FLUSH_THRESHOLD)) {
    flush_thread_local_hotspots();
  }
}

/**
 * @brief Generic operation recording with timing (lock-free)
 * OPTIMIZED: Uses thread-local buffering, no mutex in hot path
 */
OPTIWEAVE_HOTSPOT_FORCE_INLINE
void record_operation_fast(const char* op_type, const char* file, int line, 
                           const char* func, uint64_t duration_ns) {
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(!g_hotspots_enabled)) return;
  
  SourceLocation loc(file, line, func);
  
  // Fast path: record in thread-local buffer (no locking)
  auto& info = tl_hotspot_buffer.hotspots[loc];
  info.location = loc;
  info.total_time_ns += duration_ns;
  info.operation_counts[static_cast<size_t>(string_to_operation_type(op_type))]++;
  info.operation_count++;
  
  // Periodic flush to avoid unbounded memory growth
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(tl_hotspot_buffer.hotspots.size() > ThreadLocalHotspotBuffer::FLUSH_THRESHOLD)) {
    flush_thread_local_hotspots();
  }
}

/**
 * @brief Sampled operation recording - only records every Nth operation
 * This dramatically reduces overhead while still capturing hotspot data
 * OPTIMIZED: Thread-local counter, lock-free path
 */
OPTIWEAVE_HOTSPOT_FORCE_INLINE
void record_operation_sampled(const char* op_type, const char* file, int line, 
                              const char* func, uint64_t duration_ns) {
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(!g_hotspots_enabled)) return;
  
  // Thread-local sample counter - no atomic needed
  static thread_local uint32_t sample_counter = 0;
  
  // Only record every Nth operation (where N = OPTIWEAVE_HOTSPOT_SAMPLE_RATE)
  // LIKELY: we skip most samples for reduced overhead
  if (OPTIWEAVE_HOTSPOT_LIKELY(++sample_counter < OPTIWEAVE_HOTSPOT_SAMPLE_RATE)) {
    return;
  }
  sample_counter = 0;
  
  // Scale up the timing to account for sampling
  uint64_t scaled_duration = duration_ns * OPTIWEAVE_HOTSPOT_SAMPLE_RATE;
  
  SourceLocation loc(file, line, func);
  
  // Fast path: record in thread-local buffer (no locking)
  auto& info = tl_hotspot_buffer.hotspots[loc];
  info.location = loc;
  info.total_time_ns += scaled_duration;
  info.operation_counts[static_cast<size_t>(string_to_operation_type(op_type))] += OPTIWEAVE_HOTSPOT_SAMPLE_RATE;
  info.operation_count += OPTIWEAVE_HOTSPOT_SAMPLE_RATE;  // Estimate total ops
  
  // Periodic flush to avoid unbounded memory growth
  if (OPTIWEAVE_HOTSPOT_UNLIKELY(tl_hotspot_buffer.hotspots.size() > ThreadLocalHotspotBuffer::FLUSH_THRESHOLD)) {
    flush_thread_local_hotspots();
  }
}

} // namespace hotspots
} // namespace optiweave
