// src/sif.cpp
#include "sif.h"
#include "exec.h"
#include "utils.h"
#include "logger.h"
#include <fstream>
#include <sstream>

namespace sifd {

std::string SifProcessor::process(const std::string& content,
                                 const std::string& website_root,
                                 const std::map<std::string, std::string>& get_params) {
    std::string result;
    result.reserve(content.size() * 1.1); // Slightly larger for potential expansion
    
    size_t pos = 0;
    while (pos < content.size()) {
        // Look for opening <sif tag
        size_t tag_start = content.find("<sif", pos);
        
        if (tag_start == std::string::npos) {
            // No more tags, copy rest
            result += content.substr(pos);
            break;
        }
        
        // Copy everything before the tag
        result += content.substr(pos, tag_start - pos);
        
        // Find end of tag >
        size_t tag_end = content.find('>', tag_start);
        if (tag_end == std::string::npos) {
            // Malformed tag, treat as regular text
            result += content.substr(tag_start);
            break;
        }
        
        // Extract tag content (without <sif and >)
        std::string tag_content = content.substr(tag_start + 4, tag_end - tag_start - 4);
        
        // Check for self-closing tag: />
        bool self_closing = false;
        if (tag_end > tag_start && content[tag_end - 1] == '/') {
            self_closing = true;
            tag_content = tag_content.substr(0, tag_content.size() - 1);
        }
        
        // Process the tag
        result += process_tag(tag_content, website_root, get_params);
        
        // If not self-closing, look for closing </sif>
        if (!self_closing) {
            size_t close_tag = content.find("</sif>", tag_end + 1);
            if (close_tag != std::string::npos) {
                pos = close_tag + 6; // Skip past </sif>
            } else {
                pos = tag_end + 1; // No closing tag, continue after >
            }
        } else {
            pos = tag_end + 1;
        }
    }
    
    return result;
}

std::string SifProcessor::process_tag(const std::string& tag_content,
                                     const std::string& website_root,
                                     const std::map<std::string, std::string>& get_params) {
    // Trim whitespace
    std::string trimmed = utils::trim(tag_content);
    
    // Determine tag type based on attributes present
    if (trimmed.find("echo=") != std::string::npos) {
        return handle_echo(trimmed);
    } else if (trimmed.find("include=") != std::string::npos) {
        return handle_include(trimmed, website_root, get_params, 0);
    } else if (trimmed.find("read=") != std::string::npos) {
        return handle_read(trimmed);
    } else if (trimmed.find("get=") != std::string::npos) {
        return handle_get(trimmed, get_params);
    } else if (trimmed.find("exec=") != std::string::npos) {
        return handle_exec(trimmed, website_root);
    }
    
    // Unknown tag, return empty
    return "";
}

std::string SifProcessor::get_attribute(const std::string& content,
                                       const std::string& attribute) {
    std::string search = attribute + "=";
    size_t attr_pos = content.find(search);
    
    if (attr_pos == std::string::npos) {
        return "";
    }
    
    size_t value_start = attr_pos + search.length();
    
    // Handle quoted values
    if (value_start < content.length()) {
        char quote = content[value_start];
        if (quote == '"' || quote == '\'') {
            size_t value_end = content.find(quote, value_start + 1);
            if (value_end != std::string::npos) {
                return content.substr(value_start + 1, value_end - value_start - 1);
            }
        } else {
            // Unquoted value (until space or end)
            size_t value_end = content.find_first_of(" \t\r\n", value_start);
            if (value_end != std::string::npos) {
                return content.substr(value_start, value_end - value_start);
            } else {
                return content.substr(value_start);
            }
        }
    }
    
    return "";
}

std::string SifProcessor::handle_echo(const std::string& content) {
    std::string value = get_attribute(content, "echo");
    return utils::html_encode(value);
}

std::string SifProcessor::handle_include(const std::string& content,
                                        const std::string& website_root,
                                        const std::map<std::string, std::string>& get_params,
                                        int depth) {
    // Prevent infinite recursion
    if (depth >= MAX_INCLUDE_DEPTH) {
        Logger::log_error("Maximum include depth exceeded");
        return "<!-- Maximum include depth exceeded -->";
    }
    
    std::string file = get_attribute(content, "include");
    
    // Security check: prevent directory traversal
    if (utils::has_traversal(file)) {
        Logger::log_error("Include path traversal attempt: " + file);
        return "<!-- Access denied -->";
    }
    
    // Construct full path
    std::string full_path = website_root + "/" + file;
    
    // Read the file
    std::ifstream ifs(full_path.c_str(), std::ios::binary);
    if (!ifs.is_open()) {
        Logger::log_error("Include file not found: " + full_path);
        return "<!-- File not found: " + utils::html_encode(file) + " -->";
    }
    
    std::ostringstream file_content;
    file_content << ifs.rdbuf();
    ifs.close();
    
    // Recursively process the included file
    return process(file_content.str(), website_root, get_params);
}

std::string SifProcessor::handle_read(const std::string& content) {
    std::string path = get_attribute(content, "read");
    
    // Prevent directory traversal for read operations
    if (utils::has_traversal(path)) {
        Logger::log_error("Read path traversal attempt: " + path);
        return "<!-- Access denied -->";
    }
    
    // Open the file
    std::ifstream ifs(path.c_str(), std::ios::binary);
    if (!ifs.is_open()) {
        Logger::log_error("Read file not found: " + path);
        return "<!-- File not found: " + utils::html_encode(path) + " -->";
    }
    
    // Read with size limit
    std::string file_content;
    char buffer[4096];
    size_t total_read = 0;
    
    while (total_read < 1048576) { // 1MB limit
        ifs.read(buffer, sizeof(buffer));
        size_t bytes_read = ifs.gcount();
        if (bytes_read == 0) break;
        
        file_content.append(buffer, bytes_read);
        total_read += bytes_read;
    }
    
    ifs.close();
    
    // Return HTML-encoded content
    return "<pre>" + utils::html_encode(file_content) + "</pre>";
}

std::string SifProcessor::handle_get(const std::string& content,
                                    const std::map<std::string, std::string>& get_params) {
    std::string param = get_attribute(content, "get");
    
    auto it = get_params.find(param);
    if (it != get_params.end()) {
        return utils::html_encode(it->second);
    }
    
    return ""; // Parameter not found, return empty
}

std::string SifProcessor::handle_exec(const std::string& content,
                                     const std::string& website_root) {
    std::string program = get_attribute(content, "exec");
    
    // Execute the program
    Executor::ExecResult result = Executor::execute(program, website_root);
    
    if (!result.error.empty()) {
        return "<pre style=\"color: red;\">Error: " + 
               utils::html_encode(result.error) + "</pre>";
    }
    
    std::string output;
    
    // Display stdout
    if (!result.stdout_output.empty()) {
        output += "<pre>" + utils::html_encode(result.stdout_output) + "</pre>";
    }
    
    // Display stderr if present
    if (!result.stderr_output.empty()) {
        output += "<pre style=\"color: orange;\">STDERR:\n" + 
                  utils::html_encode(result.stderr_output) + "</pre>";
    }
    
    // Add execution metadata
    output += "<!-- Exec: " + utils::html_encode(program);
    output += " | Exit: " + std::to_string(result.exit_code);
    output += " | Duration: " + std::to_string(result.duration_ms) + "ms";
    output += " | Stdout: " + std::to_string(result.stdout_output.size()) + " bytes";
    output += " | Stderr: " + std::to_string(result.stderr_output.size()) + " bytes";
    output += " -->";
    
    return output;
}

} // namespace sifd