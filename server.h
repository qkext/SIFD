// src/server.h
#ifndef SIFD_SERVER_H
#define SIFD_SERVER_H

#include <string>
#include <netinet/in.h>

namespace sifd {

/**
 * Embedded TCP server for Samsung Smart TV
 * 
 * Implements a single-threaded, non-blocking TCP server using select()
 * for efficient handling of multiple connections on constrained hardware.
 * 
 * Features:
 * - Non-blocking I/O with select()
 * - Connection timeout handling
 * - Resource-efficient design for ARM Cortex-A9
 * - Graceful shutdown on SIGTERM/SIGINT
 */
class Server {
public:
    /**
     * Initialize the server
     * @param port Port number to listen on
     * @param website_root Root directory for website files
     * @param index_file Default index file name
     * @return true if initialization successful
     */
    bool init(int port, const std::string& website_root, const std::string& index_file);
    
    /**
     * Start the server main loop
     * This function blocks until shutdown signal is received
     */
    void run();
    
    /**
     * Shutdown the server gracefully
     */
    void shutdown();
    
private:
    int listen_socket_;
    int port_;
    std::string website_root_;
    std::string index_file_;
    bool running_;
    
    // Client connection tracking
    static constexpr int MAX_CLIENTS = 10;
    struct ClientConnection {
        int socket;
        std::string buffer;
        time_t last_activity;
        struct sockaddr_in address;
    };
    
    ClientConnection clients_[MAX_CLIENTS];
    
    /**
     * Initialize the listening socket
     */
    bool create_listen_socket();
    
    /**
     * Set socket to non-blocking mode
     */
    bool set_nonblocking(int socket);
    
    /**
     * Accept new client connection
     */
    void accept_client();
    
    /**
     * Handle data from a client
     * @param client_index Index into clients_ array
     */
    void handle_client(int client_index);
    
    /**
     * Close a client connection
     * @param client_index Index into clients_ array
     */
    void close_client(int client_index);
    
    /**
     * Check for timed-out connections
     */
    void check_timeouts();
    
    /**
     * Send response to client
     * @param client_index Client index
     * @param response HTTP response string
     */
    void send_response(int client_index, const std::string& response);
};

} // namespace sifd

#endif // SIFD_SERVER_H