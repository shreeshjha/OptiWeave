#include <optiweave/runtime/loop_info_serializer.hpp>
#include <fstream>
#include <cstring>
#include <iostream>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace optiweave {
namespace serialization {

namespace {

// Binary format:
// - uint32_t: number of loops
// For each loop:
//   - SourceLocation: file (string), line, function (string)
//   - int: nesting_level
//   - bool: has_divisions
//   - bool: has_constant_divisor
//   - bool: has_strided_access
//   - int: stride_value
//   - bool: is_vectorizable
//   - uint32_t: num_operations
//   - For each operation: string

void write_string(std::ofstream& out, const std::string& str) {
    uint32_t len = str.size();
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(str.data(), len);
}

std::string read_string(std::ifstream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    std::string str(len, '\0');
    in.read(&str[0], len);
    return str;
}

} // anonymous namespace

bool serialize_loop_info(
    const std::vector<analysis::LoopInfo>& loop_info,
    const std::string& filename
) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return false;
    }

    // Write number of loops
    uint32_t count = loop_info.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write each loop
    for (const auto& loop : loop_info) {
        // SourceLocation
        write_string(out, loop.location.file);
        out.write(reinterpret_cast<const char*>(&loop.location.line), sizeof(loop.location.line));
        write_string(out, loop.location.function);

        // Line range
        out.write(reinterpret_cast<const char*>(&loop.line_start), sizeof(loop.line_start));
        out.write(reinterpret_cast<const char*>(&loop.line_end), sizeof(loop.line_end));

        // Loop properties
        out.write(reinterpret_cast<const char*>(&loop.nesting_level), sizeof(loop.nesting_level));
        out.write(reinterpret_cast<const char*>(&loop.has_divisions), sizeof(loop.has_divisions));
        out.write(reinterpret_cast<const char*>(&loop.has_constant_divisor), sizeof(loop.has_constant_divisor));
        out.write(reinterpret_cast<const char*>(&loop.has_strided_access), sizeof(loop.has_strided_access));
        out.write(reinterpret_cast<const char*>(&loop.stride_value), sizeof(loop.stride_value));
        out.write(reinterpret_cast<const char*>(&loop.is_vectorizable), sizeof(loop.is_vectorizable));

        // Operations
        uint32_t num_ops = loop.operations_in_loop.size();
        out.write(reinterpret_cast<const char*>(&num_ops), sizeof(num_ops));
        for (const auto& op : loop.operations_in_loop) {
            write_string(out, op);
        }
    }

    return true;
}

bool deserialize_loop_info(
    const std::string& filename,
    std::vector<analysis::LoopInfo>& loop_info
) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        // Not an error - file may not exist if no loops were analyzed
        return false;
    }

    loop_info.clear();

    // Read number of loops
    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    // Read each loop
    for (uint32_t i = 0; i < count; ++i) {
        analysis::LoopInfo loop;

        // SourceLocation - use set_file/set_function for dynamic strings
        loop.location.set_file(read_string(in));
        in.read(reinterpret_cast<char*>(&loop.location.line), sizeof(loop.location.line));
        loop.location.set_function(read_string(in));

        // Line range
        in.read(reinterpret_cast<char*>(&loop.line_start), sizeof(loop.line_start));
        in.read(reinterpret_cast<char*>(&loop.line_end), sizeof(loop.line_end));

        // Loop properties
        in.read(reinterpret_cast<char*>(&loop.nesting_level), sizeof(loop.nesting_level));
        in.read(reinterpret_cast<char*>(&loop.has_divisions), sizeof(loop.has_divisions));
        in.read(reinterpret_cast<char*>(&loop.has_constant_divisor), sizeof(loop.has_constant_divisor));
        in.read(reinterpret_cast<char*>(&loop.has_strided_access), sizeof(loop.has_strided_access));
        in.read(reinterpret_cast<char*>(&loop.stride_value), sizeof(loop.stride_value));
        in.read(reinterpret_cast<char*>(&loop.is_vectorizable), sizeof(loop.is_vectorizable));

        // Operations
        uint32_t num_ops = 0;
        in.read(reinterpret_cast<char*>(&num_ops), sizeof(num_ops));
        for (uint32_t j = 0; j < num_ops; ++j) {
            loop.operations_in_loop.push_back(read_string(in));
        }

        loop_info.push_back(std::move(loop));
    }

    return true;
}

std::string get_loop_info_path() {
    // Use environment variable if set
    const char* env = std::getenv("OPTIWEAVE_LOOP_INFO_FILE");
    if (env) {
        return env;
    }

    // Try to find loop info in executable's directory
    // Get the path to the current executable
    std::string exe_path;

#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        exe_path = path;
    }
#elif defined(__linux__)
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path)-1);
    if (len != -1) {
        path[len] = '\0';
        exe_path = path;
    }
#endif

    if (!exe_path.empty()) {
        // Extract directory from executable path
        size_t last_slash = exe_path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            std::string exe_dir = exe_path.substr(0, last_slash);
            return exe_dir + "/.optiweave_loop_info.bin";
        }
    }

    // Fallback to current directory
    return ".optiweave_loop_info.bin";
}

} // namespace serialization
} // namespace optiweave
