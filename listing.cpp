// listing.cpp
#include "listing.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace sifd {

std::string DirectoryListing::generate(const std::string& dir_path,
                                       const std::string& url_path) {
    std::vector<DirEntry> entries;
    
    // Open directory
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return "";
    }
    
    // Read all entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        DirEntry de;
        de.name = entry->d_name;
        
        // Get file stats
        std::string full_path = dir_path + "/" + entry->d_name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            de.is_dir = S_ISDIR(st.st_mode);
            de.size = st.st_size;
            de.mtime = st.st_mtime;
        } else {
            de.is_dir = false;
            de.size = 0;
            de.mtime = 0;
        }
        
        entries.push_back(de);
    }
    
    closedir(dir);
    
    // Sort: directories first, then alphabetically
    std::sort(entries.begin(), entries.end(), compare_entries);
    
    // Generate HTML
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n<head>\n"
         << "<title>Index of " << utils::html_encode(url_path) << "</title>\n"
         << "<style>\n"
         << "body { font-family: monospace; margin: 20px; background: #f0f0f0; }\n"
         << "h1 { font-size: 1.2em; color: #333; }\n"
         << "table { border-collapse: collapse; width: 100%; background: white; }\n"
         << "th { background: #333; color: white; padding: 8px; text-align: left; }\n"
         << "td { padding: 6px 8px; border-bottom: 1px solid #ddd; }\n"
         << "tr:hover { background: #f5f5f5; }\n"
         << "a { color: #0066cc; text-decoration: none; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << ".dir { font-weight: bold; }\n"
         << "</style>\n</head>\n<body>\n";
    
    html << "<h1>Index of " << utils::html_encode(url_path) << "</h1>\n"
         << "<table>\n"
         << "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n";
    
    // Parent directory link (if not root)
    if (url_path != "/") {
        html << "<tr><td class=\"dir\"><a href=\"..\">..</a></td>"
             << "<td>-</td><td>-</td></tr>\n";
    }
    
    // List entries
    for (const auto& entry : entries) {
        html << "<tr>";
        
        if (entry.is_dir) {
            html << "<td class=\"dir\"><a href=\"" 
                 << utils::html_encode(entry.name) << "/\">"
                 << utils::html_encode(entry.name) << "/</a></td>"
                 << "<td>-</td>";
        } else {
            html << "<td><a href=\"" 
                 << utils::html_encode(entry.name) << "\">"
                 << utils::html_encode(entry.name) << "</a></td>"
                 << "<td>" << format_size(entry.size) << "</td>";
        }
        
        html << "<td>" << format_time(entry.mtime) << "</td>";
        html << "</tr>\n";
    }
    
    html << "</table>\n"
         << "<p style=\"font-size: 0.8em; color: #666;\">SIFD Server</p>\n"
         << "</body>\n</html>\n";
    
    return html.str();
}

bool DirectoryListing::compare_entries(const DirEntry& a, const DirEntry& b) {
    // Directories first
    if (a.is_dir != b.is_dir) {
        return a.is_dir; // true > false for directories
    }
    // Then alphabetical
    return a.name < b.name;
}

std::string DirectoryListing::format_size(off_t size) {
    if (size < 1024) {
        std::ostringstream oss;
        oss << size << " B";
        return oss.str();
    } else if (size < 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) 
            << (size / 1024.0) << " KB";
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) 
            << (size / (1024.0 * 1024.0)) << " MB";
        return oss.str();
    }
}

std::string DirectoryListing::format_time(time_t t) {
    if (t == 0) return "-";
    
    char buffer[64];
    struct tm* tm_info = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm_info);
    return std::string(buffer);
}

} // namespace sifd
