#include <optiweave/runtime/cache_profiler.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

// Linux-specific includes for perf_event_open
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace optiweave {
namespace runtime {

// Helper function to call perf_event_open
#ifdef __linux__
static long perf_event_open(struct perf_event_attr* hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}
#endif

void CacheStats::calculate_rates() {
    if (cache_references > 0) {
        l1_miss_rate = static_cast<double>(l1_misses) / cache_references;
        l2_miss_rate = static_cast<double>(l2_misses) / cache_references;
        l3_miss_rate = static_cast<double>(l3_misses) / cache_references;
        overall_miss_rate = static_cast<double>(total_misses) / cache_references;
    }

    if (operations > 0) {
        misses_per_operation = static_cast<double>(total_misses) / operations;
    }
}

CacheProfiler::CacheProfiler()
    : available_(false)
    , monitoring_(false)
    , fd_l1_miss_(-1)
    , fd_l2_miss_(-1)
    , fd_l3_miss_(-1)
    , fd_cache_refs_(-1) {
}

CacheProfiler::~CacheProfiler() {
    close_counters();
}

bool CacheProfiler::initialize() {
#ifdef __linux__
    // Try to setup performance counters
    fd_cache_refs_ = setup_counter(CacheEvent::CACHE_REFERENCES);
    fd_l1_miss_ = setup_counter(CacheEvent::L1_DATA_READ_MISS);
    fd_l3_miss_ = setup_counter(CacheEvent::L3_READ_MISS);

    // Check if at least cache references work
    if (fd_cache_refs_ >= 0) {
        available_ = true;
        return true;
    }

    // If we can't access perf counters, return false but don't error
    return false;
#else
    // Not supported on non-Linux platforms
    return false;
#endif
}

void CacheProfiler::start_monitoring() {
#ifdef __linux__
    if (!available_ || monitoring_) {
        return;
    }

    // Reset and enable all counters
    if (fd_cache_refs_ >= 0) {
        ioctl(fd_cache_refs_, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_cache_refs_, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (fd_l1_miss_ >= 0) {
        ioctl(fd_l1_miss_, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_l1_miss_, PERF_EVENT_IOC_ENABLE, 0);
    }
    if (fd_l3_miss_ >= 0) {
        ioctl(fd_l3_miss_, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_l3_miss_, PERF_EVENT_IOC_ENABLE, 0);
    }

    monitoring_ = true;
#endif
}

void CacheProfiler::stop_monitoring() {
#ifdef __linux__
    if (!monitoring_) {
        return;
    }

    // Disable all counters
    if (fd_cache_refs_ >= 0) {
        ioctl(fd_cache_refs_, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (fd_l1_miss_ >= 0) {
        ioctl(fd_l1_miss_, PERF_EVENT_IOC_DISABLE, 0);
    }
    if (fd_l3_miss_ >= 0) {
        ioctl(fd_l3_miss_, PERF_EVENT_IOC_DISABLE, 0);
    }

    monitoring_ = false;
#endif
}

void CacheProfiler::record_cache_stats(const char* file, uint32_t line, const char* function) {
    if (!available_ || !monitoring_) {
        return;
    }

#ifdef __linux__
    // OPTIMIZED: Use hash-based key instead of string allocation
    CacheLocationKey key(file, line, function);

    // Get or create location
    auto& location = locations_[key];
    if (location.file.empty()) {
        location.file = file;
        location.line = line;
        location.function = function;
    }

    // Read current counter values
    uint64_t cache_refs = read_counter(fd_cache_refs_);
    uint64_t l1_miss = read_counter(fd_l1_miss_);
    uint64_t l3_miss = read_counter(fd_l3_miss_);

    // Accumulate statistics
    location.stats.cache_references += cache_refs;
    location.stats.l1_misses += l1_miss;
    location.stats.l3_misses += l3_miss;
    location.stats.total_misses += (l1_miss + l3_miss);
    location.stats.operations++;

    // Recalculate rates
    location.stats.calculate_rates();
#endif
}

const CacheStats* CacheProfiler::get_stats(const char* file, uint32_t line, const char* function) const {
    CacheLocationKey key(file, line, function);
    auto it = locations_.find(key);
    if (it != locations_.end()) {
        return &it->second.stats;
    }
    return nullptr;
}

std::vector<CacheLocation> CacheProfiler::get_top_locations(size_t top_n) const {
    std::vector<CacheLocation> result;
    result.reserve(locations_.size());

    for (const auto& pair : locations_) {
        result.push_back(pair.second);
    }

    // Sort by total misses (descending)
    std::sort(result.begin(), result.end(),
              [](const CacheLocation& a, const CacheLocation& b) {
                  return a.stats.total_misses > b.stats.total_misses;
              });

    // Limit to top N if requested
    if (top_n > 0 && result.size() > top_n) {
        result.resize(top_n);
    }

    return result;
}

void CacheProfiler::print_report(size_t top_n) const {
    if (!available_) {
        std::cout << "Cache profiling not available on this system.\n";
        std::cout << "Requires Linux with perf_event support and appropriate permissions.\n";
        return;
    }

    auto top_locations = get_top_locations(top_n);

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          OptiWeave Cache Profiling Report                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    if (top_locations.empty()) {
        std::cout << "No cache profiling data collected.\n";
        return;
    }

    // Overall statistics
    uint64_t total_misses = get_total_cache_misses();
    double overall_rate = get_overall_miss_rate();

    std::cout << "Overall Cache Performance:\n";
    std::cout << "  Total Cache Misses: " << total_misses << "\n";
    std::cout << "  Overall Miss Rate: " << std::fixed << std::setprecision(2)
              << (overall_rate * 100.0) << "%\n";
    std::cout << "\n";

    // Top locations
    std::cout << "Top " << top_locations.size() << " Cache Miss Hotspots:\n";
    std::cout << "┌────┬───────────────────────────────────────────────┬──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ #  │ Location                                      │ L1 Miss  │ L3 Miss  │ Total    │ Miss/Op  │\n";
    std::cout << "├────┼───────────────────────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";

    for (size_t i = 0; i < top_locations.size(); ++i) {
        const auto& loc = top_locations[i];

        // Format location string (truncate if needed)
        std::string location_str = loc.file + ":" + std::to_string(loc.line);
        if (!loc.function.empty()) {
            location_str += " (" + loc.function + ")";
        }
        if (location_str.length() > 45) {
            location_str = location_str.substr(0, 42) + "...";
        }

        std::cout << "│ " << std::setw(2) << (i + 1) << " │ "
                  << std::left << std::setw(45) << location_str << " │ "
                  << std::right << std::setw(8) << loc.stats.l1_misses << " │ "
                  << std::setw(8) << loc.stats.l3_misses << " │ "
                  << std::setw(8) << loc.stats.total_misses << " │ "
                  << std::fixed << std::setprecision(2) << std::setw(8) << loc.stats.misses_per_operation << " │\n";
    }

    std::cout << "└────┴───────────────────────────────────────────────┴──────────┴──────────┴──────────┴──────────┘\n";
    std::cout << "\n";

    // Recommendations
    std::cout << "💡 Cache Optimization Tips:\n";
    std::cout << "  • High L3 misses indicate poor memory access patterns\n";
    std::cout << "  • Consider loop reordering for better spatial locality\n";
    std::cout << "  • Use array-of-structures (AoS) → structure-of-arrays (SoA) transformation\n";
    std::cout << "  • Apply loop blocking/tiling for large datasets\n";
    std::cout << "  • Prefetch data before use in hot loops\n";
    std::cout << "\n";
}

void CacheProfiler::export_csv(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing\n";
        return;
    }

    // CSV header
    out << "file,line,function,l1_misses,l2_misses,l3_misses,total_misses,cache_references,"
        << "operations,l1_miss_rate,l2_miss_rate,l3_miss_rate,overall_miss_rate,misses_per_op\n";

    // Write all locations
    auto locations = get_top_locations(0);
    for (const auto& loc : locations) {
        out << loc.file << ","
            << loc.line << ","
            << loc.function << ","
            << loc.stats.l1_misses << ","
            << loc.stats.l2_misses << ","
            << loc.stats.l3_misses << ","
            << loc.stats.total_misses << ","
            << loc.stats.cache_references << ","
            << loc.stats.operations << ","
            << std::fixed << std::setprecision(6)
            << loc.stats.l1_miss_rate << ","
            << loc.stats.l2_miss_rate << ","
            << loc.stats.l3_miss_rate << ","
            << loc.stats.overall_miss_rate << ","
            << loc.stats.misses_per_operation << "\n";
    }

    out.close();
}

void CacheProfiler::export_json(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing\n";
        return;
    }

    out << "{\n";
    out << "  \"summary\": {\n";
    out << "    \"available\": " << (available_ ? "true" : "false") << ",\n";
    out << "    \"total_misses\": " << get_total_cache_misses() << ",\n";
    out << "    \"overall_miss_rate\": " << get_overall_miss_rate() << ",\n";
    out << "    \"location_count\": " << locations_.size() << "\n";
    out << "  },\n";
    out << "  \"locations\": [\n";

    auto locations = get_top_locations(0);
    for (size_t i = 0; i < locations.size(); ++i) {
        const auto& loc = locations[i];

        out << "    {\n";
        out << "      \"file\": \"" << loc.file << "\",\n";
        out << "      \"line\": " << loc.line << ",\n";
        out << "      \"function\": \"" << loc.function << "\",\n";
        out << "      \"l1_misses\": " << loc.stats.l1_misses << ",\n";
        out << "      \"l2_misses\": " << loc.stats.l2_misses << ",\n";
        out << "      \"l3_misses\": " << loc.stats.l3_misses << ",\n";
        out << "      \"total_misses\": " << loc.stats.total_misses << ",\n";
        out << "      \"cache_references\": " << loc.stats.cache_references << ",\n";
        out << "      \"operations\": " << loc.stats.operations << ",\n";
        out << "      \"l1_miss_rate\": " << loc.stats.l1_miss_rate << ",\n";
        out << "      \"l2_miss_rate\": " << loc.stats.l2_miss_rate << ",\n";
        out << "      \"l3_miss_rate\": " << loc.stats.l3_miss_rate << ",\n";
        out << "      \"overall_miss_rate\": " << loc.stats.overall_miss_rate << ",\n";
        out << "      \"misses_per_operation\": " << loc.stats.misses_per_operation << "\n";
        out << "    }";

        if (i < locations.size() - 1) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    out.close();
}

uint64_t CacheProfiler::get_total_cache_misses() const {
    uint64_t total = 0;
    for (const auto& pair : locations_) {
        total += pair.second.stats.total_misses;
    }
    return total;
}

double CacheProfiler::get_overall_miss_rate() const {
    uint64_t total_misses = 0;
    uint64_t total_refs = 0;

    for (const auto& pair : locations_) {
        total_misses += pair.second.stats.total_misses;
        total_refs += pair.second.stats.cache_references;
    }

    if (total_refs > 0) {
        return static_cast<double>(total_misses) / total_refs;
    }
    return 0.0;
}

void CacheProfiler::reset() {
    locations_.clear();
}

uint64_t CacheProfiler::read_counter(int fd) const {
#ifdef __linux__
    if (fd < 0) {
        return 0;
    }

    uint64_t count = 0;
    if (read(fd, &count, sizeof(count)) != sizeof(count)) {
        return 0;
    }
    return count;
#else
    return 0;
#endif
}

int CacheProfiler::setup_counter(CacheEvent event) {
#ifdef __linux__
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    // Map event type to hardware event
    switch (event) {
        case CacheEvent::CACHE_REFERENCES:
            pe.type = PERF_TYPE_HARDWARE;
            pe.config = PERF_COUNT_HW_CACHE_REFERENCES;
            break;
        case CacheEvent::L1_DATA_READ_MISS:
            pe.type = PERF_TYPE_HW_CACHE;
            pe.config = (PERF_COUNT_HW_CACHE_L1D) |
                       (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                       (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
            break;
        case CacheEvent::L3_READ_MISS:
            pe.type = PERF_TYPE_HW_CACHE;
            pe.config = (PERF_COUNT_HW_CACHE_LL) |
                       (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                       (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
            break;
        default:
            return -1;
    }

    int fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd < 0) {
        // Permission denied or not supported
        return -1;
    }

    return fd;
#else
    return -1;
#endif
}

void CacheProfiler::close_counters() {
#ifdef __linux__
    if (fd_cache_refs_ >= 0) {
        close(fd_cache_refs_);
        fd_cache_refs_ = -1;
    }
    if (fd_l1_miss_ >= 0) {
        close(fd_l1_miss_);
        fd_l1_miss_ = -1;
    }
    if (fd_l3_miss_ >= 0) {
        close(fd_l3_miss_);
        fd_l3_miss_ = -1;
    }
#endif
}

const char* CacheProfiler::get_event_name(CacheEvent event) const {
    switch (event) {
        case CacheEvent::L1_DATA_READ_MISS: return "L1 Data Read Miss";
        case CacheEvent::L1_DATA_WRITE_MISS: return "L1 Data Write Miss";
        case CacheEvent::L2_READ_MISS: return "L2 Read Miss";
        case CacheEvent::L2_WRITE_MISS: return "L2 Write Miss";
        case CacheEvent::L3_READ_MISS: return "L3 Read Miss";
        case CacheEvent::L3_WRITE_MISS: return "L3 Write Miss";
        case CacheEvent::TOTAL_CACHE_MISSES: return "Total Cache Misses";
        case CacheEvent::CACHE_REFERENCES: return "Cache References";
        default: return "Unknown";
    }
}

// Global instance
static CacheProfiler* g_cache_profiler = nullptr;

CacheProfiler& get_cache_profiler() {
    if (!g_cache_profiler) {
        g_cache_profiler = new CacheProfiler();
        g_cache_profiler->initialize();
    }
    return *g_cache_profiler;
}

} // namespace runtime
} // namespace optiweave
