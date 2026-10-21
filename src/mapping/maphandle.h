#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

#include <ros/ros.h>

#include "../include/common/json.hpp"

using json = nlohmann::json;

class MapManager;

class MappingHandle {
public:
    using SendToCloud = std::function<void(const std::string&)>;
    using LogFileAccess = std::function<bool(const std::string&, const std::string&, int)>;
    using ScriptLogCommand = std::function<std::string(const std::string&, const std::string&)>;
    using ShellQuote = std::function<std::string(const std::string&)>;
    using LogRuntimeFlags = std::function<void(const char*)>;
    using LogWaitStatus = std::function<void(const std::string&, int)>;
    using FinishCommand = std::function<void(const std::string&)>;
    using MappingResult = std::function<void(bool, const std::string&)>;
    using ShutdownRequested = std::function<bool()>;

    MappingHandle(std::string device_id,
                  std::string scripts_path,
                  std::string map_dir,
                  std::vector<std::string> map_files,
                  MapManager* map_manager,
                  SendToCloud send_to_cloud,
                  LogFileAccess log_file_access,
                  ScriptLogCommand script_log_command,
                  ShellQuote shell_quote,
                  LogRuntimeFlags log_runtime_flags,
                  LogWaitStatus log_wait_status,
                  FinishCommand finish_command,
                  MappingResult mapping_result,
                  ShutdownRequested shutdown_requested);

    void handle_start_mapping(const json& data);
    void stop_and_uploadmap_async(json data);
    bool is_running() const;
    pid_t current_pid() const;

private:
    std::string device_id_;
    std::string scripts_path_;
    std::string map_dir_;
    std::vector<std::string> map_files_;
    pid_t mapping_process_ = -1;
    mutable std::mutex process_mutex_;
    MapManager* map_manager_ = nullptr;
    SendToCloud send_to_cloud_;
    LogFileAccess log_file_access_;
    ScriptLogCommand script_log_command_;
    ShellQuote shell_quote_;
    LogRuntimeFlags log_runtime_flags_;
    LogWaitStatus log_wait_status_;
    FinishCommand finish_command_;
    MappingResult mapping_result_;
    ShutdownRequested shutdown_requested_;
};
