// http.cpp
#include "http.h"
#include "mime.h"
#include "listing.h"
#include "sif.h"
#include "utils.h"
#include "logger.h"
#include "config.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <cstdlib>

namespace sifd {

std::string HttpHandler::process_request(const std::string& raw_request,
                                        const struct sockaddr_in& client_addr,
                                        const std::string& website_root,
                                        const std::string& index_file) {
    Request request = parse_request(raw_request);
    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    request.client_ip = ip_str;
    request.client_port = ntohs(client_addr.sin_port);
    
    if (request.method != "GET" && request.method != "HEAD") {
        Response response;
        response.status_code = 405;
        response.status_text = "Method Not Allowed";
        response.body = generate_error_page(405, "Only GET and HEAD methods are supported");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        
        Logger::log_http(request.method, request.path, 405, request.client_ip);
        return build_response(response, false);
    }
    
    if (request.path.size() > static_cast<size_t>(Config::MAX_URL_LENGTH)) {
        Response response;
        response.status_code = 414;
        response.status_text = "URI Too Long";
        response.body = generate_error_page(414, "Request URI too long");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        
        Logger::log_http(request.method, request.path, 414, request.client_ip);
        return build_response(response, false);
    }
    
    Response response = handle_file_request(request, website_root, index_file);
    
    Logger::log_http(request.method, request.path, response.status_code, request.client_ip);
    
    return build_response(response, request.method == "HEAD");
}

HttpHandler::Request HttpHandler::parse_request(const std::string& raw_request) {
    Request request;
    
    std::istringstream stream(raw_request);
    std::string line;
    
    if (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        
        std::vector<std::string> parts = utils::split(line, ' ');
        if (parts.size() >= 2) {
            request.method = parts[0];
            
            std::string full_path = parts[1];
            size_t query_pos = full_path.find('?');
            
            if (query_pos != std::string::npos) {
                request.path = utils::url_decode(full_path.substr(0, query_pos));
                request.query_string = full_path.substr(query_pos + 1);
            } else {
                request.path = utils::url_decode(full_path);
            }
        }
    }
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        
        if (line.empty()) break;
        
        if (line.size() > static_cast<size_t>(Config::MAX_HEADER_SIZE)) {
            continue;
        }
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = utils::trim(line.substr(0, colon));
            std::string value = utils::trim(line.substr(colon + 1));
            request.headers[key] = value;
        }
    }
    
    return request;
}

std::map<std::string, std::string> HttpHandler::parse_query_string(const std::string& query) {
    std::map<std::string, std::string> params;
    
    if (query.empty()) return params;
    
    std::vector<std::string> pairs = utils::split(query, '&');
    for (size_t i = 0; i < pairs.size(); ++i) {
        const std::string& pair = pairs[i];
        size_t equals = pair.find('=');
        if (equals != std::string::npos) {
            std::string key = utils::url_decode(pair.substr(0, equals));
            std::string value = utils::url_decode(pair.substr(equals + 1));
            params[key] = value;
        } else {
            std::string key = utils::url_decode(pair);
            params[key] = "";
        }
    }
    
    return params;
}

HttpHandler::Response HttpHandler::handle_file_request(const Request& request,
                                                       const std::string& website_root,
                                                       const std::string& index_file) {
    Response response;
    
    std::string sanitized_path = sanitize_path(request.path, website_root);
    if (sanitized_path.empty()) {
        response.status_code = 403;
        response.status_text = "Forbidden";
        response.body = generate_error_page(403, "Access denied");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    struct stat path_stat;
    if (stat(sanitized_path.c_str(), &path_stat) != 0) {
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = generate_error_page(404, "File not found");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    if (S_ISDIR(path_stat.st_mode)) {
        std::string index_path = sanitized_path;
        if (index_path[index_path.size() - 1] != '/') {
            index_path += '/';
        }
        index_path += index_file;
        
        if (access(index_path.c_str(), R_OK) == 0) {
            std::string ext = utils::get_extension(index_path);
            if (ext == ".sif") {
                std::map<std::string, std::string> params = parse_query_string(request.query_string);
                return serve_sif_file(index_path, params, website_root);
            } else {
                return serve_static_file(index_path);
            }
        } else {
            return handle_directory_request(sanitized_path, request.path);
        }
    } else {
        std::string ext = utils::get_extension(sanitized_path);
        if (ext == ".sif") {
            std::map<std::string, std::string> params = parse_query_string(request.query_string);
            return serve_sif_file(sanitized_path, params, website_root);
        } else {
            return serve_static_file(sanitized_path);
        }
    }
}

HttpHandler::Response HttpHandler::handle_directory_request(const std::string& dir_path,
                                                            const std::string& url_path) {
    Response response;
    
    std::string listing = DirectoryListing::generate(dir_path, url_path);
    if (listing.empty()) {
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = generate_error_page(500, "Failed to read directory");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    response.status_code = 200;
    response.status_text = "OK";
    response.content_type = "text/html; charset=utf-8";
    response.body = listing;
    response.content_length = response.body.size();
    
    return response;
}

HttpHandler::Response HttpHandler::serve_static_file(const std::string& file_path) {
    Response response;
    
    std::ifstream file(file_path.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = generate_error_page(404, "File not found");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string content(static_cast<size_t>(file_size), '\0');
    if (!file.read(&content[0], file_size)) {
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = generate_error_page(500, "Failed to read file");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    file.close();
    
    std::string ext = utils::get_extension(file_path);
    std::string mime_type = MimeTypes::get_type(ext);
    
    response.status_code = 200;
    response.status_text = "OK";
    response.content_type = mime_type;
    response.body = content;
    response.content_length = static_cast<size_t>(file_size);
    
    return response;
}

HttpHandler::Response HttpHandler::serve_sif_file(const std::string& file_path,
                                                  const std::map<std::string, std::string>& get_params,
                                                  const std::string& website_root) {
    Response response;
    
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        response.status_code = 404;
        response.status_text = "Not Found";
        response.body = generate_error_page(404, "SIF file not found");
        response.content_type = "text/html; charset=utf-8";
        response.content_length = response.body.size();
        return response;
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    file.close();
    
    std::string processed = SifProcessor::process(content.str(), website_root, get_params);
    
    response.status_code = 200;
    response.status_text = "OK";
    response.content_type = "text/html; charset=utf-8";
    response.body = processed;
    response.content_length = response.body.size();
    
    return response;
}

std::string HttpHandler::build_response(const Response& response, bool is_head) {
    std::ostringstream http_response;
    
    http_response << "HTTP/1.0 " << response.status_code << " " << response.status_text << "\r\n";
    http_response << "Server: SIFD/1.0\r\n";
    http_response << "Content-Type: " << response.content_type << "\r\n";
    http_response << "Content-Length: " << response.content_length << "\r\n";
    http_response << "Connection: close\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = response.headers.begin();
         it != response.headers.end(); ++it) {
        http_response << it->first << ": " << it->second << "\r\n";
    }
    
    http_response << "\r\n";
    
    if (!is_head) {
        http_response << response.body;
    }
    
    return http_response.str();
}

std::string HttpHandler::generate_error_page(int status_code, const std::string& message) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<title>" << status_code << " - " << message << "</title>\n"
         << "<style>\n"
         << "body { font-family: sans-serif; margin: 40px; background: #f5f5f5; }\n"
         << ".error { background: white; padding: 30px; border-radius: 5px; "
         << "box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n"
         << "h1 { color: #d32f2f; margin: 0 0 20px 0; }\n"
         << "p { color: #666; margin: 10px 0; }\n"
         << "</style>\n</head>\n<body>\n"
         << "<div class=\"error\">\n"
         << "<h1>" << status_code << " - " << message << "</h1>\n"
         << "<p>The requested resource could not be served.</p>\n"
         << "<hr>\n"
         << "<p style=\"font-size: 0.8em;\">SIFD Server</p>\n"
         << "</div>\n</body>\n</html>\n";
    
    return html.str();
}

std::string HttpHandler::sanitize_path(const std::string& path, const std::string& website_root) {
    if (utils::has_traversal(path)) {
        Logger::log_error("Path traversal attempt: " + path);
        return "";
    }
    
    std::string full_path = website_root;
    if (path.empty() || path[0] != '/') {
        full_path += '/';
    }
    full_path += path;
    
    char* resolved = realpath(full_path.c_str(), NULL);
    if (resolved == NULL) {
        size_t last_slash = full_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string parent = full_path.substr(0, last_slash);
            char* parent_resolved = realpath(parent.c_str(), NULL);
            if (parent_resolved != NULL) {
                std::string resolved_path = std::string(parent_resolved) + full_path.substr(last_slash);
                free(parent_resolved);
                
                if (resolved_path.find(website_root) == 0) {
                    return full_path;
                }
            }
        }
        return "";
    }
    
    std::string resolved_path(resolved);
    free(resolved);
    
    if (resolved_path.find(website_root) != 0) {
        Logger::log_error("Path outside website root: " + path);
        return "";
    }
    
    return resolved_path;
}

} // namespace sifd