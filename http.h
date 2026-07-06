// http.h
#ifndef SIFD_HTTP_H
#define SIFD_HTTP_H

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace sifd {

class HttpHandler {
public:
    struct Request {
        std::string method;
        std::string path;
        std::string query_string;
        std::map<std::string, std::string> headers;
        std::string client_ip;
        int client_port;
    };
    
    struct Response {
        int status_code;
        std::string status_text;
        std::string content_type;
        std::string body;
        size_t content_length;
        std::map<std::string, std::string> headers;
    };
    
    static std::string process_request(const std::string& raw_request,
                                      const struct sockaddr_in& client_addr,
                                      const std::string& website_root,
                                      const std::string& index_file);
    
    static std::string generate_error_page(int status_code, const std::string& message);

private:
    static Request parse_request(const std::string& raw_request);
    
    static std::map<std::string, std::string> parse_query_string(const std::string& query);
    
    static Response handle_file_request(const Request& request,
                                       const std::string& website_root,
                                       const std::string& index_file);
    
    static Response handle_directory_request(const std::string& dir_path,
                                            const std::string& url_path);
    
    static Response serve_static_file(const std::string& file_path);
    
    static Response serve_sif_file(const std::string& file_path,
                                  const std::map<std::string, std::string>& get_params,
                                  const std::string& website_root);
    
    static std::string build_response(const Response& response, bool is_head);
    
    static std::string sanitize_path(const std::string& path, const std::string& website_root);
};

} // namespace sifd

#endif // SIFD_HTTP_H