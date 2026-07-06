// utils.h
#ifndef SIFD_UTILS_H
#define SIFD_UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>

namespace sifd {
namespace utils {

/**
 * URL Decode function for GET parameter parsing
 */
std::string url_decode(const std::string& str);

/**
 * HTML entity encoding for safe output
 */
std::string html_encode(const std::string& text);

/**
 * Check for directory traversal attempts
 */
bool has_traversal(const std::string& path);

/**
 * Extract filename extension
 */
std::string get_extension(const std::string& filename);

/**
 * Split string by delimiter
 */
std::vector<std::string> split(const std::string& str, char delimiter);

/**
 * Trim whitespace from string
 */
std::string trim(const std::string& str);

} // namespace utils
} // namespace sifd

#endif // SIFD_UTILS_H