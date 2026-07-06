// server.cpp
#include "server.h"
#include "http.h"
#include "logger.h"
#include "config.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <cstdlib>

namespace sifd {

static Server* g_server = NULL;

static void signal_handler(int signum) {
    (void)signum;
    if (g_server) {
        g_server->shutdown();
    }
}

bool Server::init(int port, const std::string& website_root, const std::string& index_file) {
    port_ = port;
    website_root_ = website_root;
    index_file_ = index_file;
    running_ = false;
    
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients_[i].socket = -1;
        clients_[i].last_activity = 0;
    }
    
    if (!create_listen_socket()) {
        return false;
    }
    
    g_server = this;
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    return true;
}

bool Server::create_listen_socket() {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        Logger::log_error("Failed to create socket");
        return false;
    }
    
    int opt = 1;
    if (setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::log_error("Failed to set SO_REUSEADDR");
        close(listen_socket_);
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(listen_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::log_error("Failed to bind to port");
        close(listen_socket_);
        return false;
    }
    
    if (listen(listen_socket_, 5) < 0) {
        Logger::log_error("Failed to listen");
        close(listen_socket_);
        return false;
    }
    
    if (!set_nonblocking(listen_socket_)) {
        close(listen_socket_);
        return false;
    }
    
    return true;
}

bool Server::set_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }
    
    return true;
}

void Server::run() {
    running_ = true;
    
    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        
        FD_SET(listen_socket_, &read_fds);
        int max_fd = listen_socket_;
        
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients_[i].socket >= 0) {
                FD_SET(clients_[i].socket, &read_fds);
                if (clients_[i].socket > max_fd) {
                    max_fd = clients_[i].socket;
                }
            }
        }
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            Logger::log_error("select failed");
            break;
        }
        
        if (FD_ISSET(listen_socket_, &read_fds)) {
            accept_client();
        }
        
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients_[i].socket >= 0 && FD_ISSET(clients_[i].socket, &read_fds)) {
                handle_client(i);
            }
        }
        
        check_timeouts();
    }
    
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients_[i].socket >= 0) {
            close_client(i);
        }
    }
    
    close(listen_socket_);
}

void Server::accept_client() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_socket = accept(listen_socket_, (struct sockaddr*)&client_addr, &addr_len);
    if (client_socket < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::log_error("Accept failed");
        }
        return;
    }
    
    if (!set_nonblocking(client_socket)) {
        close(client_socket);
        return;
    }
    
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients_[i].socket < 0) {
            clients_[i].socket = client_socket;
            clients_[i].buffer.clear();
            clients_[i].last_activity = time(NULL);
            clients_[i].address = client_addr;
            return;
        }
    }
    
    Logger::log_error("Too many connections, rejecting");
    close(client_socket);
}

void Server::handle_client(int client_index) {
    char buffer[4096];
    ssize_t bytes_read = recv(clients_[client_index].socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        close_client(client_index);
        return;
    }
    
    buffer[bytes_read] = '\0';
    clients_[client_index].buffer += buffer;
    clients_[client_index].last_activity = time(NULL);
    
    // Fix: Use static_cast for comparison
    if (clients_[client_index].buffer.size() > static_cast<size_t>(Config::MAX_REQUEST_SIZE)) {
        Logger::log_error("Request too large, closing connection");
        close_client(client_index);
        return;
    }
    
    size_t header_end = clients_[client_index].buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return;
    }
    
    std::string headers = clients_[client_index].buffer.substr(0, header_end + 4);
    
    std::string response = HttpHandler::process_request(
        headers,
        clients_[client_index].address,
        website_root_,
        index_file_
    );
    
    send_response(client_index, response);
    close_client(client_index);
}

void Server::send_response(int client_index, const std::string& response) {
    size_t total_sent = 0;
    size_t remaining = response.size();
    
    while (remaining > 0) {
        ssize_t sent = send(clients_[client_index].socket,
                           response.c_str() + total_sent,
                           remaining,
                           0);
        
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            }
            Logger::log_error("Send failed");
            break;
        }
        
        total_sent += static_cast<size_t>(sent);
        remaining -= static_cast<size_t>(sent);
    }
}

void Server::close_client(int client_index) {
    if (clients_[client_index].socket >= 0) {
        close(clients_[client_index].socket);
        clients_[client_index].socket = -1;
        clients_[client_index].buffer.clear();
        clients_[client_index].last_activity = 0;
    }
}

void Server::check_timeouts() {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients_[i].socket >= 0) {
            if (now - clients_[i].last_activity > Config::CONNECTION_TIMEOUT_SEC) {
                Logger::log_error("Connection timeout");
                close_client(i);
            }
        }
    }
}

void Server::shutdown() {
    running_ = false;
}

} // namespace sifd