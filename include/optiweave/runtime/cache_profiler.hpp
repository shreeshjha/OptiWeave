#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace optiweave {
namespace runtime {

/**
 * @brief Cache event types we can monitor
 */
enum class CacheEvent {
    L1_DATA_READ_MISS,
    L1_DATA_WRITE_MISS,
    L2_READ_MISS,
    L2_WRITE_MISS,
    L3_READ_MISS,
    L3_WRITE_MISS,
    TOTAL_CACHE_MISSES,
    CACHE_REFERENCES
};

/**
 * @brief Cache statistics for a specific location
 */
struct CacheStats {
    uint64_t l1_misses = 0;
    uint64_t l2_misses = 0;
    uint64_t l3_misses = 0;
    uint64_t cache_references = 0;
    uint64_t total_misses = 0;
    uint64_t operations = 0;

    // Computed metrics
    double l1_miss_rate = 0.0;
    double l2_miss_rate = 0.0;
    double l3_miss_rate = 0.0;
    double overall_miss_rate = 0.0;
    double misses_per_operation = 0.0;

    void calculate_rates();
};

/**
 * @brief Fast location key for cache profiler map lookup
 * OPTIMIZED: Uses pre-computed FNV-1a hash, avoids string allocation
 */
struct CacheLocationKey {
    const char* file;
    uint32_t line;
    const char* function;
    size_t hash_cache;
    
    CacheLocationKey() : file(""), line(0), function(""), hash_cache(0) {}
    
    CacheLocationKey(const char* f, uint32_t l, const char* fn) 
        : file(f), line(l), function(fn), hash_cache(compute_hash(f, l, fn)) {}
    
    static size_t compute_hash(const char* f, uint32_t l, const char* fn) {
        // FNV-1a hash - fast and good distribution
        constexpr size_t FNV_PRIME = 1099511628211ULL;
        constexpr size_t FNV_OFFSET = 14695981039346656037ULL;
        
        size_t h = FNV_OFFSET;
        h ^= reinterpret_cast<size_t>(f);
        h *= FNV_PRIME;
        h ^= static_cast<size_t>(l);
        h *= FNV_PRIME;
        h ^= reinterpret_cast<size_t>(fn);
        h *= FNV_PRIME;
        return h;
    }
    
    bool operator==(const CacheLocationKey& other) const {
        // Fast path: compare pointers and line (usually sufficient for static strings)
        if (file == other.file && line == other.line && function == other.function) {
            return true;
        }
        // Slow path: compare string contents
        return line == other.line && 
               std::strcmp(file, other.file) == 0 && 
               std::strcmp(function, other.function) == 0;
    }
};

} // namespace runtime
} // namespace optiweave

// Hash specialization for CacheLocationKey - must be in std namespace
namespace std {
template<> struct hash<optiweave::runtime::CacheLocationKey> {
    size_t operator()(const optiweave::runtime::CacheLocationKey& key) const {
        return key.hash_cache;
    }
};
} // namespace std

namespace optiweave {
namespace runtime {

/**
 * @brief Location-specific cache profiling data
 */
struct CacheLocation {
    std::string file;
    uint32_t line;
    std::string function;
    CacheStats stats;

    CacheLocation() = default;
    CacheLocation(const std::string& f, uint32_t l, const std::string& func)
        : file(f), line(l), function(func) {}
    CacheLocation(const char* f, uint32_t l, const char* func)
        : file(f), line(l), function(func) {}
};

/**
 * @brief Cache profiler using Linux perf_event_open API
 *
 * This class provides low-overhead cache miss tracking by leveraging
 * hardware performance counters. It requires Linux kernel support
 * and appropriate permissions (CAP_PERFMON or /proc/sys/kernel/perf_event_paranoid).
 */
class CacheProfiler {
public:
    CacheProfiler();
    ~CacheProfiler();

    // Disable copy/move
    CacheProfiler(const CacheProfiler&) = delete;
    CacheProfiler& operator=(const CacheProfiler&) = delete;
    CacheProfiler(CacheProfiler&&) = delete;
    CacheProfiler& operator=(CacheProfiler&&) = delete;

    /**
     * @brief Initialize cache profiling
     * @return true if successfully initialized, false otherwise
     */
    bool initialize();

    /**
     * @brief Check if cache profiling is available on this system
     * @return true if perf_event is available
     */
    bool is_available() const { return available_; }

    /**
     * @brief Start cache event monitoring
     */
    void start_monitoring();

    /**
     * @brief Stop cache event monitoring
     */
    void stop_monitoring();

    /**
     * @brief Record cache statistics for a specific location
     * @param file Source file name
     * @param line Line number
     * @param function Function name
     */
    void record_cache_stats(const char* file, uint32_t line, const char* function);

    /**
     * @brief Get cache statistics for a specific location
     * @param file Source file name
     * @param line Line number
     * @param function Function name
     * @return Cache statistics or nullptr if not found
     */
    const CacheStats* get_stats(const char* file, uint32_t line, const char* function) const;

    /**
     * @brief Get all cache locations sorted by total misses
     * @param top_n Number of top locations to return (0 = all)
     * @return Vector of cache locations
     */
    std::vector<CacheLocation> get_top_locations(size_t top_n = 0) const;

    /**
     * @brief Print cache profiling report to stdout
     * @param top_n Number of top locations to show
     */
    void print_report(size_t top_n = 10) const;

    /**
     * @brief Export cache profiling data to CSV
     * @param filename Output file name
     */
    void export_csv(const std::string& filename) const;

    /**
     * @brief Export cache profiling data to JSON
     * @param filename Output file name
     */
    void export_json(const std::string& filename) const;

    /**
     * @brief Get total cache misses across all locations
     * @return Total cache misses
     */
    uint64_t get_total_cache_misses() const;

    /**
     * @brief Get overall cache miss rate
     * @return Miss rate (0.0 - 1.0)
     */
    double get_overall_miss_rate() const;

    /**
     * @brief Reset all cache statistics
     */
    void reset();

private:
    bool available_;
    bool monitoring_;

    // Performance counter file descriptors
    int fd_l1_miss_;
    int fd_l2_miss_;
    int fd_l3_miss_;
    int fd_cache_refs_;

    // Location-based cache statistics - OPTIMIZED: uses hash key instead of string
    std::unordered_map<CacheLocationKey, CacheLocation> locations_;

    /**
     * @brief Read current value from performance counter
     * @param fd File descriptor
     * @return Counter value
     */
    uint64_t read_counter(int fd) const;

    /**
     * @brief Setup performance counter for specific event
     * @param event Cache event type
     * @return File descriptor or -1 on error
     */
    int setup_counter(CacheEvent event);

    /**
     * @brief Close all performance counters
     */
    void close_counters();

    /**
     * @brief Get event name string for reporting
     */
    const char* get_event_name(CacheEvent event) const;
};

/**
 * @brief Global cache profiler instance
 *
 * This is used by the instrumented code to record cache statistics.
 * It's initialized lazily on first use.
 */
CacheProfiler& get_cache_profiler();

} // namespace runtime
} // namespace optiweave
