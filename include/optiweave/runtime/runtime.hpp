#pragma once

/**
 * @file runtime.hpp
 * @brief OptiWeave Runtime Library API
 * 
 * This header provides C and C++ APIs for configuring the OptiWeave runtime
 * instrumentation system.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the OptiWeave runtime with default settings
 * 
 * This function is called automatically when the runtime library is loaded,
 * but can be called manually to reset to default configuration.
 */
void __optiweave_init(void);

/**
 * @brief Set the log file path and enable file logging
 * @param filepath Path to the log file (NULL to disable file logging)
 */
void __optiweave_set_log_file(const char* filepath);

/**
 * @brief Enable or disable file logging
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_file_logging(int enable);

/**
 * @brief Enable or disable stderr logging
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_stderr_logging(int enable);

/**
 * @brief Enable or disable array access logging
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_array_logging(int enable);

/**
 * @brief Enable or disable arithmetic operation logging
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_arithmetic_logging(int enable);

/**
 * @brief Enable or disable timestamps in log messages
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_timestamps(int enable);

/**
 * @brief Enable or disable file/line location info in log messages
 * @param enable Non-zero to enable, zero to disable
 */
void __optiweave_enable_location_info(int enable);

#ifdef __cplusplus
}

// C++ convenience API
namespace optiweave {

/**
 * @brief C++ wrapper for runtime configuration
 */
class Runtime {
public:
    static void init() { __optiweave_init(); }
    static void setLogFile(const std::string& filepath) { 
        __optiweave_set_log_file(filepath.c_str()); 
    }
    static void enableFileLogging(bool enable = true) { 
        __optiweave_enable_file_logging(enable ? 1 : 0); 
    }
    static void enableStderrLogging(bool enable = true) { 
        __optiweave_enable_stderr_logging(enable ? 1 : 0); 
    }
    static void enableArrayLogging(bool enable = true) { 
        __optiweave_enable_array_logging(enable ? 1 : 0); 
    }
    static void enableArithmeticLogging(bool enable = true) { 
        __optiweave_enable_arithmetic_logging(enable ? 1 : 0); 
    }
    static void enableTimestamps(bool enable = true) { 
        __optiweave_enable_timestamps(enable ? 1 : 0); 
    }
    static void enableLocationInfo(bool enable = true) { 
        __optiweave_enable_location_info(enable ? 1 : 0); 
    }
};

} // namespace optiweave

#endif // __cplusplus