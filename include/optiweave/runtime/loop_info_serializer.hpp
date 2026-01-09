#pragma once

#include <optiweave/analysis/pattern_detector.hpp>
#include <string>
#include <vector>

namespace optiweave {
namespace serialization {

/// Serialize loop information to a binary file
bool serialize_loop_info(
    const std::vector<analysis::LoopInfo>& loop_info,
    const std::string& filename
);

/// Deserialize loop information from a binary file
bool deserialize_loop_info(
    const std::string& filename,
    std::vector<analysis::LoopInfo>& loop_info
);

/// Get default loop info file path based on executable name
std::string get_loop_info_path();

} // namespace serialization
} // namespace optiweave
