// src/logger.h
#ifndef SIFD_LOGGER_H
#define SIFD_LOGGER_H

#include <string>
#include <mutex>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/time.h>

namespace sifd {

/**
 * Thread-safe logging system for embedded HTTP server
 * 
 * Provides persistent logging with timestamps for:
 * - HTTP request/response logging
 * - Binary execution logging
 * - Error conditions
 * - Security violations
 * 
 * Designed for minimal filesystem impact on flash storage
 */
class Logger {
public:
    /**
     * Initialize logger with output file path
     * @param log_path Full path to log file
     * @return true if file opened successfully
     */
    static bool init(const std::string& log_path);
    
    /**
     * Log HTTP request details
     * @param method HTTP method (GET, HEAD)
     * @param path Request path
     * @param status HTTP status code
     * @param client_ip Client IP address
     */
    static void log_http(const std::string& method,
                        const std::string& path,
                        int status,
                        const std::string& client_ip);
    
    /**
     * Log binary execution details
     * @param program Executed program name
     * @param exit_code Process exit code
     * @param duration_ms Execution time in milliseconds
     * @param stdout_size Bytes captured from stdout
     * @param stderr_size Bytes captured from stderr
     */
    static void log_exec(const std::string& program,
                        int exit_code,
                        long duration_ms,
                        size_t stdout_size,
                        size_t stderr_size);
    
    /**
     * Log error conditions
     * @param error Error description
     */
    static void log_error(const std::string& error);
    
    /**
     * Close the logger
     */
    static void shutdown();

private:
    static std::ofstream log_file_;
    static std::mutex mutex_;
    
    /**
     * Get current timestamp string [HH:MM:SS]
     */
    static std::string get_timestamp();
};

} // namespace sifd

#endif // SIFD_LOGGER_H