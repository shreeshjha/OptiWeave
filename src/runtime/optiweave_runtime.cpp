#include "../../templates/prelude.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>

// Define the global configuration instance
namespace optiweave {
    InstrumentationConfig g_config;
}

// Thread-safe logging with mutex
static std::mutex log_mutex;

// Helper to get current timestamp
static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Helper to format log message
static std::string formatLogMessage(const std::string& operation, 
                                   const std::string& details,
                                   const char* file, int line) {
    std::stringstream ss;
    
    if (optiweave::g_config.include_timestamps) {
        ss << "[" << getCurrentTimestamp() << "] ";
    }
    
    ss << "OptiWeave: " << operation;
    
    if (!details.empty()) {
        ss << " " << details;
    }
    
    if (optiweave::g_config.include_location && file) {
        // Extract just the filename, not the full path
        const char* filename = strrchr(file, '/');
        if (!filename) filename = strrchr(file, '\\');
        if (!filename) filename = file;
        else filename++; // Skip the slash
        
        ss << " (" << filename << ":" << line << ")";
    }
    
    return ss.str();
}

// Helper to write log message to appropriate destinations
static void writeLogMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    if (optiweave::g_config.log_to_stderr) {
        std::cerr << message << std::endl;
    }
    
    if (optiweave::g_config.log_to_file && !optiweave::g_config.log_file_path.empty()) {
        std::ofstream logfile(optiweave::g_config.log_file_path, std::ios::app);
        if (logfile.is_open()) {
            logfile << message << std::endl;
        }
    }
}

// Implement the C logging functions
extern "C" {
    
void __optiweave_log_access(const char *operation, const void *ptr,
                            std::size_t index, const char *file, int line) {
    if (!optiweave::g_config.log_array_accesses) {
        return;
    }
    
    std::stringstream details;
    details << "at " << ptr << "[" << index << "]";
    
    std::string message = formatLogMessage(
        operation ? operation : "access", 
        details.str(), 
        file, 
        line
    );
    
    writeLogMessage(message);
}

void __optiweave_log_operation(const char *operation, const char *lhs_type,
                               const char *rhs_type, const char *file, int line) {
    if (!optiweave::g_config.log_arithmetic_ops) {
        return;
    }
    
    std::stringstream details;
    if (lhs_type && rhs_type) {
        details << "(" << lhs_type << " " << operation << " " << rhs_type << ")";
    }
    
    std::string message = formatLogMessage(
        operation ? operation : "operation",
        details.str(),
        file,
        line
    );
    
    writeLogMessage(message);
}

// Additional utility functions for runtime configuration
void __optiweave_init() {
    // Initialize with default configuration
    optiweave::g_config.log_array_accesses = true;
    optiweave::g_config.log_arithmetic_ops = false;
    optiweave::g_config.log_to_stderr = true;
    optiweave::g_config.log_to_file = false;
    optiweave::g_config.log_file_path = "optiweave.log";
    optiweave::g_config.include_timestamps = true;
    optiweave::g_config.include_location = true;
}

void __optiweave_set_log_file(const char* filepath) {
    if (filepath) {
        optiweave::g_config.log_file_path = filepath;
        optiweave::g_config.log_to_file = true;
    }
}

void __optiweave_enable_file_logging(int enable) {
    optiweave::g_config.log_to_file = (enable != 0);
}

void __optiweave_enable_stderr_logging(int enable) {
    optiweave::g_config.log_to_stderr = (enable != 0);
}

void __optiweave_enable_array_logging(int enable) {
    optiweave::g_config.log_array_accesses = (enable != 0);
}

void __optiweave_enable_arithmetic_logging(int enable) {
    optiweave::g_config.log_arithmetic_ops = (enable != 0);
}

void __optiweave_enable_timestamps(int enable) {
    optiweave::g_config.include_timestamps = (enable != 0);
}

void __optiweave_enable_location_info(int enable) {
    optiweave::g_config.include_location = (enable != 0);
}

} // extern "C"

// C++ initialization helper
namespace optiweave {

class RuntimeInitializer {
public:
    RuntimeInitializer() {
        __optiweave_init();
    }
};

// Global initializer - will be called before main()
static RuntimeInitializer g_initializer;

} // namespace optiweave