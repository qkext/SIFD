#include "utils.h"
#include <cstdlib>

namespace sifd {
namespace utils {

std::string url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            char hex[3] = {str[i+1], str[i+2], '\0'};
            char* endptr;
            long val = strtol(hex, &endptr, 16);
            if (endptr == hex + 2) {
                result += static_cast<char>(val);
                i += 2;
                continue;
            }
        } else if (str[i] == '+') {
            result += ' ';
            continue;
        }
        result += str[i];
    }
    
    return result;
}

std::string html_encode(const std::string& text) {
    std::string encoded;
    encoded.reserve(text.size() * 2);
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        switch (c) {
            case '&':  encoded += "&amp;"; break;
            case '<':  encoded += "&lt;"; break;
            case '>':  encoded += "&gt;"; break;
            case '"':  encoded += "&quot;"; break;
            case '\'': encoded += "&#39;"; break;
            default:   encoded += c;
        }
    }
    
    return encoded;
}

bool has_traversal(const std::string& path) {
    // Check for "../" patterns (directory traversal)
    if (path.find("..") != std::string::npos) {
        return true;
    }
    
    // Check for null bytes (used to truncate strings)
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '\0') {
            return true;
        }
    }
    
    // Check for double slashes (may bypass filters)
    if (path.find("//") != std::string::npos) {
        return true;
    }
    
    // REMOVED: Don't block absolute paths - HTTP paths start with /
    // The sanitize_path function handles path resolution safely
    
    return false;
}

std::string get_extension(const std::string& filename) {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    
    std::string ext = filename.substr(dot);
    for (size_t i = 0; i < ext.size(); ++i) {
        ext[i] = tolower(ext[i]);
    }
    return ext;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    
    tokens.push_back(str.substr(start));
    return tokens;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

} // namespace utils
} // namespace sifd
