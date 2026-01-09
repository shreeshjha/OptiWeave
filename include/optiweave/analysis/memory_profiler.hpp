#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include <map>
#include <string>
#include <vector>

namespace optiweave {
namespace analysis {

/**
 * @brief Represents a memory allocation site
 */
struct AllocationSite {
  std::string file;
  unsigned line;
  std::string function;
  std::string allocation_type;  // "new", "new[]", "malloc", "calloc", etc.
  bool is_array;
  std::string element_type;

  AllocationSite() : line(0), is_array(false) {}
};

/**
 * @brief Represents a memory deallocation site
 */
struct DeallocationSite {
  std::string file;
  unsigned line;
  std::string function;
  std::string deallocation_type;  // "delete", "delete[]", "free"
  bool is_array;

  DeallocationSite() : line(0), is_array(false) {}
};

/**
 * @brief Memory profiling statistics
 */
struct MemoryStatistics {
  size_t total_allocations = 0;
  size_t total_deallocations = 0;
  size_t array_allocations = 0;
  size_t scalar_allocations = 0;
  size_t array_deallocations = 0;
  size_t scalar_deallocations = 0;
  size_t malloc_calls = 0;
  size_t calloc_calls = 0;
  size_t realloc_calls = 0;
  size_t free_calls = 0;
  size_t potential_leaks = 0;  // allocations without matching deallocations
  size_t potential_double_frees = 0;  // deallocations without matching allocations
};

/**
 * @brief Memory profiler for static analysis
 *
 * Analyzes C++ code to detect memory allocations and deallocations,
 * identifies potential memory leaks, and generates profiling data.
 */
class MemoryProfiler {
public:
  MemoryProfiler() = default;

  /**
   * @brief Add an allocation site
   */
  void add_allocation(const AllocationSite& site);

  /**
   * @brief Add a deallocation site
   */
  void add_deallocation(const DeallocationSite& site);

  /**
   * @brief Analyze memory usage patterns
   */
  void analyze();

  /**
   * @brief Get memory statistics
   */
  const MemoryStatistics& get_statistics() const { return stats_; }

  /**
   * @brief Get all allocation sites
   */
  const std::vector<AllocationSite>& get_allocations() const {
    return allocations_;
  }

  /**
   * @brief Get all deallocation sites
   */
  const std::vector<DeallocationSite>& get_deallocations() const {
    return deallocations_;
  }

  /**
   * @brief Export memory profile to JSON
   */
  void export_json(const std::string& filename) const;

  /**
   * @brief Export memory profile to text
   */
  void export_text(const std::string& filename) const;

  /**
   * @brief Print statistics to output stream
   */
  void print_statistics(llvm::raw_ostream& os) const;

  /**
   * @brief Merge another profiler's data into this one
   */
  void merge(const MemoryProfiler& other);

private:
  std::vector<AllocationSite> allocations_;
  std::vector<DeallocationSite> deallocations_;
  MemoryStatistics stats_;

  /**
   * @brief Detect potential memory leaks
   */
  void detect_leaks();

  /**
   * @brief Detect potential double frees
   */
  void detect_double_frees();
};

} // namespace analysis
} // namespace optiweave
