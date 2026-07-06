// src/main.cpp
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include "server.h"
#include "logger.h"
#include "config.h"

/**
 * SIFD - Samsung Interactive File Daemon
 * 
 * Lightweight embedded HTTP server for Samsung Smart TVs.
 * Designed for VDLinux on Novatek NT72668 ARM Cortex-A9.
 * 
 * Command line options:
 *   -f <path>    Website root directory
 *   -p <port>    Port number to listen on
 *   --index <file> Default index file (default: index.sif)
 *   --log <file>   Log file path
 *   --help         Show help
 */

static void print_usage(const char* program_name) {
    std::cout << "SIFD - Samsung Interactive File Daemon v1.0\n"
              << "Usage: " << program_name << " [options]\n\n"
              << "Options:\n"
              << "  -f <path>      Website root directory (required)\n"
              << "  -p <port>      Port number (default: 8080)\n"
              << "  --index <file> Default index file (default: index.sif)\n"
              << "  --log <file>   Log file path (default: /mtd_rwarea/sifd.log)\n"
              << "  --help         Show this help\n\n"
              << "Example:\n"
              << "  " << program_name << " -f /mtd_rwarea/website -p 5555 "
              << "--index index.sif --log /mtd_rwarea/sifd.log\n\n"
              << "Target: Samsung Smart TV (VDLinux, ARM Cortex-A9)\n"
              << "Compiler: arm-v7a8v4r3-linux-gnueabi-g++\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Default configuration
    std::string website_root;
    int port = sifd::Config::DEFAULT_PORT;
    std::string index_file = sifd::Config::DEFAULT_INDEX;
    std::string log_file = sifd::Config::DEFAULT_LOG;
    
    // Define long options
    static struct option long_options[] = {
        {"index", required_argument, 0, 'i'},
        {"log",   required_argument, 0, 'l'},
        {"help",  no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Parse command line arguments
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "f:p:i:l:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                website_root = optarg;
                break;
                
            case 'p':
                port = std::atoi(optarg);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Error: Invalid port number: " << optarg << std::endl;
                    return 1;
                }
                break;
                
            case 'i':
                index_file = optarg;
                break;
                
            case 'l':
                log_file = optarg;
                break;
                
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Validate required parameters
    if (website_root.empty()) {
        std::cerr << "Error: Website root directory is required (-f option)" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if website root exists
    if (access(website_root.c_str(), R_OK) != 0) {
        std::cerr << "Error: Cannot access website root: " << website_root << std::endl;
        return 1;
    }
    
    // Initialize logger
    if (!sifd::Logger::init(log_file)) {
        std::cerr << "Warning: Cannot open log file: " << log_file << std::endl;
        std::cerr << "Continuing without logging..." << std::endl;
    }
    
    // Log startup information
    sifd::Logger::log_error("SIFD starting");
    sifd::Logger::log_error("Website root: " + website_root);
    sifd::Logger::log_error("Port: " + std::to_string(port));
    sifd::Logger::log_error("Index file: " + index_file);
    
    // Print startup banner (for console/debug)
    std::cout << "SIFD v1.0 - Samsung Interactive File Daemon\n"
              << "Website root: " << website_root << "\n"
              << "Port: " << port << "\n"
              << "Index: " << index_file << "\n"
              << "Log: " << log_file << "\n"
              << "Starting server...\n"
              << "Press Ctrl+C to stop\n" << std::endl;
    
    // Create and start server
    sifd::Server server;
    
    if (!server.init(port, website_root, index_file)) {
        std::cerr << "Error: Failed to initialize server" << std::endl;
        sifd::Logger::log_error("Failed to initialize server");
        sifd::Logger::shutdown();
        return 1;
    }
    
    // Run the server (this blocks until shutdown)
    server.run();
    
    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    sifd::Logger::log_error("SIFD stopped");
    sifd::Logger::shutdown();
    
    return 0;
}