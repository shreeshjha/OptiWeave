#include <optiweave/analysis/memory_profiler.hpp>

#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace optiweave {
namespace analysis {

void MemoryProfiler::add_allocation(const AllocationSite& site) {
  allocations_.push_back(site);
}

void MemoryProfiler::add_deallocation(const DeallocationSite& site) {
  deallocations_.push_back(site);
}

void MemoryProfiler::analyze() {
  // Count allocations
  stats_.total_allocations = allocations_.size();
  stats_.array_allocations = std::count_if(
      allocations_.begin(), allocations_.end(),
      [](const AllocationSite& site) { return site.is_array; });
  stats_.scalar_allocations = stats_.total_allocations - stats_.array_allocations;

  // Count specific allocation types
  stats_.malloc_calls = std::count_if(
      allocations_.begin(), allocations_.end(),
      [](const AllocationSite& site) { return site.allocation_type == "malloc"; });
  stats_.calloc_calls = std::count_if(
      allocations_.begin(), allocations_.end(),
      [](const AllocationSite& site) { return site.allocation_type == "calloc"; });
  stats_.realloc_calls = std::count_if(
      allocations_.begin(), allocations_.end(),
      [](const AllocationSite& site) { return site.allocation_type == "realloc"; });

  // Count deallocations
  stats_.total_deallocations = deallocations_.size();
  stats_.array_deallocations = std::count_if(
      deallocations_.begin(), deallocations_.end(),
      [](const DeallocationSite& site) { return site.is_array; });
  stats_.scalar_deallocations = stats_.total_deallocations - stats_.array_deallocations;

  // Count free calls
  stats_.free_calls = std::count_if(
      deallocations_.begin(), deallocations_.end(),
      [](const DeallocationSite& site) { return site.deallocation_type == "free"; });

  // Detect issues
  detect_leaks();
  detect_double_frees();
}

void MemoryProfiler::detect_leaks() {
  // Simple heuristic: more allocations than deallocations suggests potential leaks
  // Note: This is a static approximation, runtime tracking would be more accurate
  if (stats_.total_allocations > stats_.total_deallocations) {
    stats_.potential_leaks = stats_.total_allocations - stats_.total_deallocations;
  }
}

void MemoryProfiler::detect_double_frees() {
  // Simple heuristic: more deallocations than allocations suggests potential double-frees
  if (stats_.total_deallocations > stats_.total_allocations) {
    stats_.potential_double_frees = stats_.total_deallocations - stats_.total_allocations;
  }
}

void MemoryProfiler::export_json(const std::string& filename) const {
  std::ofstream out(filename);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + filename);
  }

  out << "{\n";
  out << "  \"statistics\": {\n";
  out << "    \"total_allocations\": " << stats_.total_allocations << ",\n";
  out << "    \"total_deallocations\": " << stats_.total_deallocations << ",\n";
  out << "    \"array_allocations\": " << stats_.array_allocations << ",\n";
  out << "    \"scalar_allocations\": " << stats_.scalar_allocations << ",\n";
  out << "    \"array_deallocations\": " << stats_.array_deallocations << ",\n";
  out << "    \"scalar_deallocations\": " << stats_.scalar_deallocations << ",\n";
  out << "    \"malloc_calls\": " << stats_.malloc_calls << ",\n";
  out << "    \"calloc_calls\": " << stats_.calloc_calls << ",\n";
  out << "    \"realloc_calls\": " << stats_.realloc_calls << ",\n";
  out << "    \"free_calls\": " << stats_.free_calls << ",\n";
  out << "    \"potential_leaks\": " << stats_.potential_leaks << ",\n";
  out << "    \"potential_double_frees\": " << stats_.potential_double_frees << "\n";
  out << "  },\n";

  // Export allocations
  out << "  \"allocations\": [\n";
  for (size_t i = 0; i < allocations_.size(); ++i) {
    const auto& alloc = allocations_[i];
    out << "    {\n";
    out << "      \"file\": \"" << alloc.file << "\",\n";
    out << "      \"line\": " << alloc.line << ",\n";
    out << "      \"function\": \"" << alloc.function << "\",\n";
    out << "      \"type\": \"" << alloc.allocation_type << "\",\n";
    out << "      \"is_array\": " << (alloc.is_array ? "true" : "false") << ",\n";
    out << "      \"element_type\": \"" << alloc.element_type << "\"\n";
    out << "    }";
    if (i < allocations_.size() - 1) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";

  // Export deallocations
  out << "  \"deallocations\": [\n";
  for (size_t i = 0; i < deallocations_.size(); ++i) {
    const auto& dealloc = deallocations_[i];
    out << "    {\n";
    out << "      \"file\": \"" << dealloc.file << "\",\n";
    out << "      \"line\": " << dealloc.line << ",\n";
    out << "      \"function\": \"" << dealloc.function << "\",\n";
    out << "      \"type\": \"" << dealloc.deallocation_type << "\",\n";
    out << "      \"is_array\": " << (dealloc.is_array ? "true" : "false") << "\n";
    out << "    }";
    if (i < deallocations_.size() - 1) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";

  out.close();
}

void MemoryProfiler::export_text(const std::string& filename) const {
  std::ofstream out(filename);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + filename);
  }

  out << "=== Memory Profiling Report ===\n\n";

  // Statistics
  out << "Memory Statistics:\n";
  out << "  Total Allocations: " << stats_.total_allocations << "\n";
  out << "    Scalar (new): " << stats_.scalar_allocations << "\n";
  out << "    Array (new[]): " << stats_.array_allocations << "\n";
  out << "    malloc: " << stats_.malloc_calls << "\n";
  out << "    calloc: " << stats_.calloc_calls << "\n";
  out << "    realloc: " << stats_.realloc_calls << "\n";
  out << "\n";
  out << "  Total Deallocations: " << stats_.total_deallocations << "\n";
  out << "    Scalar (delete): " << stats_.scalar_deallocations << "\n";
  out << "    Array (delete[]): " << stats_.array_deallocations << "\n";
  out << "    free: " << stats_.free_calls << "\n";
  out << "\n";

  // Issues
  if (stats_.potential_leaks > 0 || stats_.potential_double_frees > 0) {
    out << "Potential Issues:\n";
    if (stats_.potential_leaks > 0) {
      out << "  ⚠️  Potential Memory Leaks: " << stats_.potential_leaks << " allocation(s) without matching deallocation\n";
    }
    if (stats_.potential_double_frees > 0) {
      out << "  ⚠️  Potential Double-Frees: " << stats_.potential_double_frees << " deallocation(s) without matching allocation\n";
    }
    out << "\n";
  }

  // Allocation details
  if (!allocations_.empty()) {
    out << "Allocation Sites (" << allocations_.size() << "):\n";
    for (const auto& alloc : allocations_) {
      out << "  [" << alloc.file << ":" << alloc.line << "] ";
      out << alloc.allocation_type;
      if (alloc.is_array) {
        out << "[] ";
      } else {
        out << " ";
      }
      out << alloc.element_type;
      if (!alloc.function.empty()) {
        out << " in " << alloc.function;
      }
      out << "\n";
    }
    out << "\n";
  }

  // Deallocation details
  if (!deallocations_.empty()) {
    out << "Deallocation Sites (" << deallocations_.size() << "):\n";
    for (const auto& dealloc : deallocations_) {
      out << "  [" << dealloc.file << ":" << dealloc.line << "] ";
      out << dealloc.deallocation_type;
      if (dealloc.is_array) {
        out << "[]";
      }
      if (!dealloc.function.empty()) {
        out << " in " << dealloc.function;
      }
      out << "\n";
    }
  }

  out.close();
}

void MemoryProfiler::print_statistics(llvm::raw_ostream& os) const {
  os << "\n";
  os << "╔══════════════════════════════════════════════════════════════╗\n";
  os << "║          OptiWeave Memory Profiling Statistics               ║\n";
  os << "╚══════════════════════════════════════════════════════════════╝\n";
  os << "\n";

  os << "Memory Operations:\n";
  os << "  Allocations:   " << stats_.total_allocations << "\n";
  os << "    new:         " << stats_.scalar_allocations << "\n";
  os << "    new[]:       " << stats_.array_allocations << "\n";
  os << "    malloc:      " << stats_.malloc_calls << "\n";
  os << "    calloc:      " << stats_.calloc_calls << "\n";
  os << "    realloc:     " << stats_.realloc_calls << "\n";
  os << "\n";
  os << "  Deallocations: " << stats_.total_deallocations << "\n";
  os << "    delete:      " << stats_.scalar_deallocations << "\n";
  os << "    delete[]:    " << stats_.array_deallocations << "\n";
  os << "    free:        " << stats_.free_calls << "\n";
  os << "\n";

  // Balance check
  int balance = static_cast<int>(stats_.total_allocations) -
                static_cast<int>(stats_.total_deallocations);
  os << "  Balance:       ";
  if (balance > 0) {
    os << "+" << balance << " (potential leaks)\n";
  } else if (balance < 0) {
    os << balance << " (potential double-frees)\n";
  } else {
    os << "0 (balanced)\n";
  }
  os << "\n";

  // Warnings
  if (stats_.potential_leaks > 0 || stats_.potential_double_frees > 0) {
    os << "⚠️  Warnings:\n";
    if (stats_.potential_leaks > 0) {
      os << "  • " << stats_.potential_leaks << " potential memory leak(s)\n";
    }
    if (stats_.potential_double_frees > 0) {
      os << "  • " << stats_.potential_double_frees << " potential double-free(s)\n";
    }
    os << "\n";
  }
}

void MemoryProfiler::merge(const MemoryProfiler& other) {
  // Merge allocations
  allocations_.insert(allocations_.end(),
                     other.allocations_.begin(),
                     other.allocations_.end());

  // Merge deallocations
  deallocations_.insert(deallocations_.end(),
                       other.deallocations_.begin(),
                       other.deallocations_.end());

  // Re-analyze after merging
  analyze();
}

} // namespace analysis
} // namespace optiweave
