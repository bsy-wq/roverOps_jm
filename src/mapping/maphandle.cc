#include "maphandle.h"

#include "../../include/common/log_manager.h"
#include "../../include/manager.h"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

namespace fs = std::filesystem;

MappingHandle::MappingHandle(std::string device_id,
                             std::string scripts_path,
                             std::string map_dir,
                             std::vector<std::string> map_files,
                             pid_t& mapping_process,
                             std::mutex& process_mutex,
                             MapManager* map_manager,
                             SendToCloud send_to_cloud,
                             LogFileAccess log_file_access,
                             ScriptLogCommand script_log_command,
                             ShellQuote shell_quote,
                             LogRuntimeFlags log_runtime_flags,
                             LogWaitStatus log_wait_status,
                             FinishCommand finish_command)
    : device_id_(std::move(device_id)),
      scripts_path_(std::move(scripts_path)),
      map_dir_(std::move(map_dir)),
      map_files_(std::move(map_files)),
      mapping_process_(mapping_process),
      process_mutex_(process_mutex),
      map_manager_(map_manager),
      send_to_cloud_(std::move(send_to_cloud)),
      log_file_access_(std::move(log_file_access)),
      script_log_command_(std::move(script_log_command)),
      shell_quote_(std::move(shell_quote)),
      log_runtime_flags_(std::move(log_runtime_flags)),
      log_wait_status_(std::move(log_wait_status)),
      finish_command_(std::move(finish_command)) {}

void MappingHandle::handle_start_mapping(const json& data) {
    ROS_INFO("    ======================= 开始建图 (handle_start_mapping) =======================    ");

    fs::remove("/home/jianmi/g_build_map/all_raw_points.pcd");
    fs::remove("/home/jianmi/g_build_map/gloabl_map.pcd");
    fs::remove("/home/jianmi/g_build_map/gloabl_map.pickle");

    LOG_INFO(" 开始建图 (handle_start_mapping) ... ");
    LOG_INFO("[Mapping] start_mapping begin, payload={}", data.dump());
    if (log_runtime_flags_) {
        log_runtime_flags_("start_mapping_begin");
    }

    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        if (mapping_process_ > 0) {
            ROS_WARN("Mapping process is already running.");
            LOG_WARN("[Mapping] start ignored: mapping process is already running, pid={}",
                     mapping_process_);
            if (send_to_cloud_) {
                send_to_cloud_(json{
                    {"type", "start_mapping"},
                    {"code", -1},
                    {"localTime", getTimeNow()},
                    {"message", "建图程序已经运行, 请勿重复操作... "}
                }.dump());
            }
            return;
        }
    }

    ROS_INFO("Starting mapping scripts (mapping.sh)...");

    const std::string mapping_script =
        (fs::path(scripts_path_) / "mapping.sh").string();
    const std::string mapping_command =
        script_log_command_ ? script_log_command_(mapping_script, "mapping")
                            : mapping_script;
    LOG_INFO("[Mapping] script path: {}", mapping_script);

    if (!log_file_access_ ||
        !log_file_access_("mapping.sh", mapping_script, F_OK | R_OK)) {
        ROS_ERROR("Script not found: %s", mapping_script.c_str());
        if (send_to_cloud_) {
            send_to_cloud_(json{
                {"type", "start_mapping"},
                {"code", -2},
                {"localTime", getTimeNow()},
                {"message", "建图脚本不存在 ... "}
            }.dump());
        }
        return;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        ROS_ERROR("Fork failed");
        LOG_ERROR("[Mapping] fork failed for mapping.sh, errno={}, error={}",
                  errno, std::strerror(errno));
        return;
    }

    if (pid == 0) {
        setpgid(0, 0);
        std::cout << "[ start mapping ] PID: " << getpid() << std::endl;

        execl("/bin/bash", "bash", "-c", mapping_command.c_str(), (char*)nullptr);

        std::fprintf(
            stderr,
            "[Mapping] execl failed for mapping.sh, path=%s, errno=%d, error=%s\n",
            mapping_script.c_str(), errno, std::strerror(errno));
        perror("execl failed");
        std::exit(1);
    }

    std::cout << "[Parent] fork success, child PID: " << pid << std::endl;
    LOG_INFO("[Mapping] mapping.sh started, pid={}", pid);
    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        mapping_process_ = pid;
    }

    const json ack = {
        {"type", "ack_start_mapping"},
        {"code", 0},
        {"device_id", device_id_},
        {"status", "success"},
        {"localTime", getTimeNow()},
        {"timestamp", static_cast<int>(std::time(nullptr))}
    };
    if (send_to_cloud_) {
        send_to_cloud_(ack.dump());
    }
    LOG_INFO("开始路径上报: {}", ack.dump());
    if (log_runtime_flags_) {
        log_runtime_flags_("start_mapping_success");
    }
}

void MappingHandle::stop_and_uploadmap_async(json data) {
    ROS_INFO("    ======================= 停止建图并上传地址 (handlea_stop_uploadmap) =======================    ");
    LOG_INFO("停止建图并上传地址 (handlea_stop_uploadmap)");
    LOG_INFO("[Mapping] stop_and_uploadmap begin, payload={}", data.dump());
    if (log_runtime_flags_) {
        log_runtime_flags_("stop_and_uploadmap_begin");
    }
    std::string display_name = data.value("display_name", "Unnamed Map");
    std::string address = data.value("address", "");

    pid_t mapping_pid = -1;
    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        mapping_pid = mapping_process_;
    }

    if (mapping_pid > 0) {
        ROS_INFO("Stopping mapping process...");
        LOG_INFO("[Mapping] send SIGINT to mapping.sh process group, pid={}", mapping_pid);

        kill(-mapping_pid, SIGINT);
        int mapping_status = 0;
        waitpid(mapping_pid, &mapping_status, 0); // 等待主进程退出
        if (log_wait_status_) {
            log_wait_status_("mapping.sh", mapping_status);
        }
        {
            std::lock_guard<std::mutex> lock(process_mutex_);
            if (mapping_process_ == mapping_pid) {
                mapping_process_ = -1;
            }
        }
  
        std::this_thread::sleep_for(std::chrono::seconds(2));

        ROS_INFO("Running post-processing script...");

        std::string process_script = (fs::path(scripts_path_) / "map_process.sh").string();
        std::cout << "bsy ... " << process_script << std::endl;
        LOG_INFO("[Mapping] post process script path: {}", process_script);

        // 检查脚本是否存在
        if (!log_file_access_ ||
            !log_file_access_("map_process.sh", process_script, F_OK | R_OK)) {
            ROS_ERROR("Script not found: %s", process_script.c_str());
            if (log_runtime_flags_) {
                log_runtime_flags_("stop_and_uploadmap_done");
            }
            if (finish_command_) {
                finish_command_("stop_and_uploadmap");
            }
            return;
        }

        // 确保可执行（保险）
        std::string chmod_cmd = "chmod +x " + process_script;
        LOG_INFO("[Mapping] chmod command: {}", chmod_cmd);
        int chmod_ret = system(chmod_cmd.c_str());
        if (chmod_ret != 0) {
            LOG_ERROR("[Mapping] chmod failed, command={}, status={}", chmod_cmd, chmod_ret);
            if (log_wait_status_) {
                log_wait_status_("chmod map_process.sh", chmod_ret);
            }
        }

        const std::string process_command =
            script_log_command_ ? script_log_command_(process_script, "map_process")
                                : process_script;
        const std::string process_cmd =
            "/bin/bash -c " +
            (shell_quote_ ? shell_quote_(process_command) : process_command);
        LOG_INFO("[Mapping] post process command: {}", process_cmd);
        int ret = system(process_cmd.c_str());
        LOG_INFO("[Mapping] post process command finished, status={}", ret);

        if (ret == 0) {
            ROS_INFO("Map processing completed successfully.");
            LOG_INFO("[Mapping] post process success");

            std::string upload_path = "/home/jianmi/g_build_map";
            auto [storage_path, uuid] = map_manager_
                ? map_manager_->upload_map(device_id_, address, display_name, upload_path, map_files_)
                : std::pair<std::string, std::string>{"", ""};

            std::string status_msg = storage_path.empty()
                ? "completed_but_upload_failed"
                : "completed_and_uploaded";

            if (!storage_path.empty()) {
                ROS_INFO("Map uploaded to server successfully!");
                status_msg = "completed_and_uploaded";
                LOG_INFO("[Mapping] upload success, storage_path={}, uuid={}", storage_path, uuid);
            } else {
                ROS_ERROR("Map processing succeeded but upload failed.");
                status_msg = "completed_but_upload_failed";
                LOG_ERROR("[Mapping] upload failed after post process, address={}, display_name={}, map_dir={}",
                          address, display_name, map_dir_);
            }

            json msg3 = {
                {"type", "ack_stop_and_uploadmap"},
                {"device_id", device_id_},
                {"display_name", display_name},
                {"address", address},
                {"status", status_msg},
                {"storage_path", storage_path},
                {"code",0},
                {"uuid", uuid},
                {"map_type", "high"},
                {"localTime", getTimeNow()},
                {"timestamp", (int)time(nullptr)}
            };
            send_to_cloud_(msg3.dump());
            // std::cout << "uuid --------------> " << uuid << std::endl;
            LOG_INFO("开始路径上报: {}", msg3.dump());
            std::cout << "发送 开始并停止建图成功 ... " << std::endl;
        } else {
            ROS_ERROR("Post-processing script failed, aborting upload.");
            LOG_ERROR("[Mapping] post process failed, script={}, status={}", process_script, ret);
            if (log_wait_status_) {
                log_wait_status_("map_process.sh", ret);
            }
        }

    } else {
        ROS_WARN("No mapping process currently running.");
        LOG_WARN("[Mapping] stop_and_uploadmap ignored: no mapping process running");
    }
    if (log_runtime_flags_) {
        log_runtime_flags_("stop_and_uploadmap_done");
    }
    if (finish_command_) {
        finish_command_("stop_and_uploadmap");
    }
}
