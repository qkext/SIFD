// exec.cpp
#include "exec.h"
#include "utils.h"
#include "logger.h"
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <climits>
#include <unistd.h>
#include <string>
#include <sstream>

namespace sifd {

Executor::ExecResult Executor::execute(const std::string& program,
                                       const std::string& website_root) {
    ExecResult result;
    result.exit_code = -1;
    result.duration_ms = 0;
    result.timed_out = false;
    
    std::string exec_path = validate_path(program, website_root);
    if (exec_path.empty()) {
        result.error = "Invalid path";
        Logger::log_error("Exec security violation: " + program);
        return result;
    }
    
    if (access(exec_path.c_str(), X_OK) != 0) {
        result.error = "Program not found: " + program;
        Logger::log_error(result.error);
        return result;
    }
    
    int stdout_pipe[2], stderr_pipe[2];
    if (!setup_pipes(stdout_pipe, stderr_pipe)) {
        result.error = "Failed to create pipes";
        Logger::log_error(result.error);
        return result;
    }
    
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        result.error = "Fork failed";
        Logger::log_error(result.error);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return result;
        
    } else if (pid == 0) {
        // Child process
        dup2(stdout_pipe[PIPE_WRITE], STDOUT_FILENO);
        dup2(stderr_pipe[PIPE_WRITE], STDERR_FILENO);
        
        close(stdout_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]);
        close(stderr_pipe[PIPE_READ]);
        close(stderr_pipe[PIPE_WRITE]);
        
        // Run through /bin/sh for script support
        char* const argv[] = {
            const_cast<char*>("/bin/sh"),
            const_cast<char*>(exec_path.c_str()),
            NULL
        };
        
        execv("/bin/sh", argv);
        _exit(127);
        
    } else {
        // Parent process
        close(stdout_pipe[PIPE_WRITE]);
        close(stderr_pipe[PIPE_WRITE]);
        
        int flags = fcntl(stdout_pipe[PIPE_READ], F_GETFL, 0);
        fcntl(stdout_pipe[PIPE_READ], F_SETFL, flags | O_NONBLOCK);
        flags = fcntl(stderr_pipe[PIPE_READ], F_GETFL, 0);
        fcntl(stderr_pipe[PIPE_READ], F_SETFL, flags | O_NONBLOCK);
        
        bool child_done = false;
        int status;
        
        while (!child_done) {
            pid_t waited = waitpid(pid, &status, WNOHANG);
            
            if (waited == pid) {
                child_done = true;
                
                if (WIFEXITED(status)) {
                    result.exit_code = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    result.exit_code = -1;
                    std::ostringstream oss;
                    oss << "Signal: " << WTERMSIG(status);
                    result.error = oss.str();
                }
                break;
                
            } else if (waited == -1) {
                result.error = "waitpid failed";
                break;
            }
            
            gettimeofday(&current_time, NULL);
            result.duration_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                                (current_time.tv_usec - start_time.tv_usec) / 1000;
            
            if (result.duration_ms >= 5000) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                result.timed_out = true;
                result.error = "Execution timeout (5 seconds)";
                child_done = true;
                break;
            }
            
            usleep(10000);
        }
        
        if (!child_done) {
            usleep(100000);
        }
        
        result.stdout_output = read_pipe(stdout_pipe[PIPE_READ], MAX_OUTPUT);
        result.stderr_output = read_pipe(stderr_pipe[PIPE_READ], MAX_OUTPUT);
        
        close(stdout_pipe[PIPE_READ]);
        close(stderr_pipe[PIPE_READ]);
        
        Logger::log_exec(program, result.exit_code, result.duration_ms,
                        result.stdout_output.size(), result.stderr_output.size());
        
        if (!result.error.empty()) {
            Logger::log_error("Exec " + program + ": " + result.error);
        }
    }
    
    return result;
}

std::string Executor::validate_path(const std::string& program,
                                   const std::string& website_root) {
    if (program.empty()) {
        return "";
    }
    
    if (utils::has_traversal(program)) {
        return "";
    }
    
    if (program.find('/') != std::string::npos) {
        return "";
    }
    
    if (program.find_first_of(";&|`$(){}[]!") != std::string::npos) {
        return "";
    }
    
    std::string full_path = website_root + "/bin/" + program;
    return full_path;
}

bool Executor::setup_pipes(int stdout_pipe[2], int stderr_pipe[2]) {
    if (pipe(stdout_pipe) == -1) {
        return false;
    }
    
    if (pipe(stderr_pipe) == -1) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return false;
    }
    
    return true;
}

std::string Executor::read_pipe(int fd, size_t max_size) {
    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (output.size() + static_cast<size_t>(bytes_read) > max_size) {
            size_t remaining = max_size - output.size();
            output.append(buffer, remaining);
            break;
        }
        output.append(buffer, bytes_read);
    }
    
    return output;
}

} // namespace sifd