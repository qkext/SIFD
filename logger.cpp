// src/logger.cpp
#include "logger.h"

namespace sifd {

std::ofstream Logger::log_file_;
std::mutex Logger::mutex_;

bool Logger::init(const std::string& log_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    log_file_.open(log_path.c_str(), std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
        return false;
    }
    
    // Write startup marker
    log_file_ << "[" << get_timestamp() << "] SIFD started" << std::endl;
    log_file_.flush();
    
    return true;
}

void Logger::log_http(const std::string& method,
                     const std::string& path,
                     int status,
                     const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!log_file_.is_open()) return;
    
    log_file_ << "[" << get_timestamp() << "] "
              << method << " " << path << " "
              << status << " "
              << client_ip << std::endl;
    log_file_.flush();
}

void Logger::log_exec(const std::string& program,
                     int exit_code,
                     long duration_ms,
                     size_t stdout_size,
                     size_t stderr_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!log_file_.is_open()) return;
    
    log_file_ << "[" << get_timestamp() << "] EXEC " << program << std::endl
              << "  exit=" << exit_code << std::endl
              << "  duration=" << duration_ms << "ms" << std::endl
              << "  stdout=" << stdout_size << " bytes" << std::endl
              << "  stderr=" << stderr_size << " bytes" << std::endl;
    log_file_.flush();
}

void Logger::log_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!log_file_.is_open()) return;
    
    log_file_ << "[" << get_timestamp() << "] ERROR: " << error << std::endl;
    log_file_.flush();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (log_file_.is_open()) {
        log_file_ << "[" << get_timestamp() << "] SIFD stopped" << std::endl;
        log_file_.close();
    }
}

std::string Logger::get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << tm_info->tm_hour << ":"
        << std::setw(2) << tm_info->tm_min << ":"
        << std::setw(2) << tm_info->tm_sec;
    
    return oss.str();
}

} // namespace sifd