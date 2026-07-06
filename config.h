// config.h
#ifndef SIFD_CONFIG_H
#define SIFD_CONFIG_H

#include <cstddef>
#include <string>

namespace sifd {

/**
 * SIFD Configuration Constants
 * 
 * All tunable parameters for the daemon are centralized here
 * to make embedded system optimization straightforward.
 */
struct Config {
    // Server defaults
    static constexpr int DEFAULT_PORT = 8080;
    static constexpr const char* DEFAULT_ROOT = "/mtd_rwarea/website";
    static constexpr const char* DEFAULT_INDEX = "index.sif";
    static constexpr const char* DEFAULT_LOG = "/mtd_rwarea/sifd.log";
    
    // Connection limits (embedded constraints)
    static constexpr int MAX_CONNECTIONS = 10;
    static constexpr int CONNECTION_TIMEOUT_SEC = 30;
    static constexpr int MAX_HEADER_SIZE = 8192;    // 8KB
    static constexpr int MAX_URL_LENGTH = 2048;      // 2KB
    static constexpr int MAX_REQUEST_SIZE = 65536;   // 64KB
    
    // Execution constraints
    static constexpr int EXEC_TIMEOUT_SEC = 5;
    static constexpr size_t MAX_EXEC_OUTPUT = 1048576;  // 1MB
    static constexpr size_t MAX_READ_FILE = 1048576;    // 1MB
    
    // Buffer sizes
    static constexpr size_t FILE_BUFFER_SIZE = 8192;
    static constexpr size_t SEND_BUFFER_SIZE = 16384;
    
    // Security restrictions
    static constexpr const char* EXEC_DIR = "bin/";
    
    // Prevent instantiation
    Config() = delete;
};

} // namespace sifd

#endif // SIFD_CONFIG_H
