// exec.h
#ifndef SIFD_EXEC_H
#define SIFD_EXEC_H

#include <string>
#include <sys/types.h>

namespace sifd {

class Executor {
public:
    struct ExecResult {
        std::string stdout_output;
        std::string stderr_output;
        int exit_code;
        long duration_ms;
        bool timed_out;
        std::string error;
    };
    
    static ExecResult execute(const std::string& program,
                             const std::string& website_root);

private:
    static const int PIPE_READ = 0;
    static const int PIPE_WRITE = 1;
    static const size_t MAX_OUTPUT = 1048576; // 1MB
    
    static std::string validate_path(const std::string& program,
                                    const std::string& website_root);
    
    static bool setup_pipes(int stdout_pipe[2], int stderr_pipe[2]);
    
    static std::string read_pipe(int fd, size_t max_size);
};

} // namespace sifd

#endif // SIFD_EXEC_H
