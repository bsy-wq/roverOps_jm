#pragma once

#include <chrono>
#include <csignal>
#include <functional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace process_runner {

// Runs a shell command in its own process group and waits with a deadline.
// Returns the raw wait status (0 on success) or -1 on spawn/timeout failure.
// When should_stop() becomes true, the process group is terminated so program
// shutdown is not held hostage by a long-running external script.
inline int run_shell_command(const std::string& command,
                             std::chrono::milliseconds timeout,
                             const std::function<bool()>& should_stop = {}) {
    const pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        setpgid(0, 0);
        execl("/bin/bash", "bash", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    // Avoid a race where the child has not called setpgid() yet.
    setpgid(pid, pid);

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    while (true) {
        const pid_t ret = waitpid(pid, &status, WNOHANG);
        if (ret == pid) return status;
        if (ret < 0) return -1;

        if (std::chrono::steady_clock::now() >= deadline ||
            (should_stop && should_stop())) {
            kill(-pid, SIGTERM);
            kill(pid, SIGTERM);
            for (int attempt = 0; attempt < 10; ++attempt) {
                const pid_t term = waitpid(pid, &status, WNOHANG);
                if (term == pid) return status;
                if (term < 0) return -1;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            kill(-pid, SIGKILL);
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return status;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

}  // namespace process_runner
