#include "../include/CommandExecutor.h"

#include "gateway/gateway.h"
#include "../include/common/log_manager.h"
#include "manager.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <filesystem>
#include <limits>
#include <std_msgs/Int32.h>

void CommandExecutor::setGateway(gateway* gw) {
    gateway_ = gw;
    if (gateway_) {
        gateway_->setConnectedCallback([this]() {
            try_flush_msgs();
        });
    }
    try_flush_msgs();
}

CommandExecutor::CommandExecutor(ros::NodeHandle& nh, 
                                        const std::string& device_id, const std::string& http_ip, 
                                        const int& http_port,
                                        const std::string& scripts_path, const std::string& map_dir,
                                        const std::vector<std::string>& map_files,
                                        const char* bms_model,
                                        tinyxml2::XMLDocument& doc,
                                        const std::string& parameter_path, const std::string& parameter_yaml_path)
    : nh_(nh),
      device_id_(device_id),
      http_ip_(http_ip),
      http_port_(http_port),
      bms_topic_(bms_model && bms_model[0] != '\0' ? bms_model : "/m20/bms_state"),
      scripts_path_(scripts_path),
      map_dir_(map_dir),
      action_timeout_sec_(300),
      map_files_(map_files),
      doc_(doc),
      parameter_path_(parameter_path),
      parameter_yaml_path_(parameter_yaml_path),
      modify_handle_(doc_, 
        [this](const std::string& message) {
            send_to_cloud(message);
        }, device_id_, parameter_path_, parameter_yaml_path_) {
    navigation_env_ready_.store(false, std::memory_order_release);
    localization_system_ready_.store(false, std::memory_order_release);
    upload_enable_.store(false, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_map_id_ = "map_001";
    }
    nav_process_ = -1;
    goal_process_ = -1;
    mapping_process_ = -1;
    cleanup_process_ = -1;

    map_manager_ = std::make_unique<MapManager>(http_ip, http_port);
    map_manager_->set_progress_callback([this](const std::string& msg) {
        send_to_cloud(msg);
    });

    mapping_handle_ = std::make_unique<MappingHandle>(
        device_id_,
        scripts_path_,
        map_dir_,
        map_files_,
        mapping_process_,
        process_mutex_,
        map_manager_.get(),
        [this](const std::string& message) {
            send_to_cloud(message);
        },
        [this](const std::string& name, const std::string& path, int mode) {
            return log_file_access(name, path, mode);
        },
        [this](const std::string& script_path, const std::string& tag) {
            return script_log_command(script_path, tag);
        },
        [this](const std::string& value) {
            return shell_quote(value);
        },
        [this](const char* stage) {
            log_runtime_flags(stage);
        },
        [this](const std::string& name, int status) {
            log_wait_status(name, status);
        },
        [this](const std::string& cmd_type) {
            finish_command(cmd_type);
        });

    robot_state_handle_ = std::make_unique<robotState>(
        nh_,
        device_id_,
        [this](const std::string& message) {
            send_to_cloud(message);
        }
    );

    motion_handle_ = std::make_unique<MotionHandle>(nh_);

    std::string hikvision_ip = "192.168.44.64";
    std::string hikvision_username = "admin";
    std::string hikvision_password = "yuanqi456";
    int hikvision_channel = 1;
    nh_.param<std::string>("hikvision_ip", hikvision_ip, hikvision_ip);
    nh_.param<std::string>("hikvision_username", hikvision_username, hikvision_username);
    nh_.param<std::string>("hikvision_password", hikvision_password, hikvision_password);
    nh_.param<int>("hikvision_channel", hikvision_channel, hikvision_channel);
    hikvision_handle_ = std::make_unique<HikvisionHandle>(
        hikvision_ip,
        hikvision_username,
        hikvision_password,
        hikvision_channel);

    init_ros_publishers();
    init_ros_subscribers();
}

CommandExecutor::~CommandExecutor() {
    if (robot_state_handle_) {
        robot_state_handle_->clear_navigation_environment();
    }

    if (telemetry_thread_.joinable()) {
        telemetry_thread_.join();
    }

    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(worker_mutex_);
        workers = std::move(background_workers_);
    }

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // A background navigation worker may have published a late completion
    // result while the destructor was waiting for it.
    if (robot_state_handle_) {
        robot_state_handle_->clear_navigation_environment();
    }

    if (gateway_) {
        gateway_->setConnectedCallback(std::function<void()>{});
    }
}

void CommandExecutor::init_ros_publishers() {
    initial_pose_pub_ = nh_.advertise<geometry_msgs::PoseWithCovarianceStamped>("/initialpose", 1, true);
    loops_pub_ = nh_.advertise<std_msgs::Int8>("/navigation_loops", 1, true);
    wait_times_pub_ = nh_.advertise<std_msgs::Int32MultiArray>("/navigation_wait_times", 1, true);
    control_pub_ = nh_.advertise<std_msgs::Int8>("/control_mode", 10);
    goal_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/goal", 10);
    dock_pose_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/dock_pose", 1, true);
    cycles_pub_ = nh_.advertise<std_msgs::Int32>("/set_nav_cycles", 1, true);

    nh_.param<std::string>("robot_model", robot_model_, "M20");
}

void CommandExecutor::init_ros_subscribers() {
    loc_sub_ = nh_.subscribe("/localization", 10, &CommandExecutor::localization_callback, this);
    global_path_sub_ = nh_.subscribe("/neupan_path", 10, &CommandExecutor::global_path_callback, this);
    rosout_sub_ = nh_.subscribe("/rosout", 50, &CommandExecutor::initial_pose_rosout_callback, this);
    if (robot_state_handle_) {
        robot_state_ = nh_.subscribe(
            "/robot_state",
            10,
            &robotState::read_ros_state_info_callback,
            robot_state_handle_.get());
        bms_info_ = nh_.subscribe(
            "/m20/bms_state",
            10,
            &robotState::read_ros_bms_info_callback,
            robot_state_handle_.get());
        LOG_INFO("[RobotState] subscribed /robot_state, /m20/bms_state");
    }
}

void CommandExecutor::send_to_cloud(const std::string& msg) {
    constexpr size_t kMaxPendingMsgs = 50;
    const bool is_upload_position = msg.find("\"type\":\"upload_position\"") != std::string::npos;
    const bool is_robot_state_info = msg.find("\"type\":\"robot_state_info\"") != std::string::npos;

    {
        std::lock_guard<std::mutex> lock(msg_mutex_);

        if (is_robot_state_info) {
            pending_msgs_.erase(
                std::remove_if(
                    pending_msgs_.begin(),
                    pending_msgs_.end(),
                    [](const std::string& pending_msg) {
                        return pending_msg.find("\"type\":\"robot_state_info\"") != std::string::npos;
                    }),
                pending_msgs_.end());
        }

        if (is_upload_position) {
            while (pending_msgs_.size() >= kMaxPendingMsgs && !pending_msgs_.empty()) {
                const bool front_is_upload =
                    pending_msgs_.front().find("\"type\":\"upload_position\"") != std::string::npos;
                if (!front_is_upload) {
                    break;
                }
                pending_msgs_.pop_front();
            }

            if (pending_msgs_.size() >= kMaxPendingMsgs) {
                return;
            }
        }

        if (is_robot_state_info && pending_msgs_.size() >= kMaxPendingMsgs) {
            return;
        }

        pending_msgs_.push_back(msg);
    }
    try_flush_msgs();
}

bool CommandExecutor::should_send_upload_position() {
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const long long interval_ms =
        upload_position_interval_ms_.load(std::memory_order_acquire);

    long long last_ms = last_upload_position_ms_.load(std::memory_order_acquire);
    while (true) {
        if (now_ms - last_ms < interval_ms) {
            return false;
        }
        if (last_upload_position_ms_.compare_exchange_weak(
                last_ms, now_ms,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return true;
        }
    }
}

void CommandExecutor::handle_set_position_upload_frequency(const json& data) {
    const json& payload =
        data.contains("data") && data["data"].is_object()
            ? data["data"]
            : data;

    double frequency_hz = 0.0;
    bool valid = false;
    if (payload.contains("frequency_hz")) {
        try {
            const json& raw_frequency = payload["frequency_hz"];
            if (raw_frequency.is_number()) {
                frequency_hz = raw_frequency.get<double>();
                valid = true;
            } else if (raw_frequency.is_string()) {
                size_t parsed_chars = 0;
                const std::string text = raw_frequency.get<std::string>();
                frequency_hz = std::stod(text, &parsed_chars);
                valid = parsed_chars == text.size();
            }
        } catch (...) {
            valid = false;
        }
    }

    valid = valid && std::isfinite(frequency_hz) && frequency_hz > 0.0 &&
            frequency_hz <= 1000.0;
    if (!valid) {
        send_to_cloud(json{
            {"type", "ack_set_position_upload_frequency"},
            {"code", 400},
            {"status", "failed"},
            {"localTime", getTimeNow()},
            {"message", "frequency_hz must be a finite number in (0, 1000]"}
        }.dump());
        LOG_WARN("[PositionUpload] invalid frequency command: {}", data.dump());
        return;
    }

    const long long interval_ms = std::max<long long>(
        1, static_cast<long long>(std::llround(1000.0 / frequency_hz)));
    upload_position_interval_ms_.store(interval_ms, std::memory_order_release);
    last_upload_position_ms_.store(0, std::memory_order_release);

    send_to_cloud(json{
        {"type", "ack_set_position_upload_frequency"},
        {"code", 0},
        {"status", "success"},
        {"frequency_hz", frequency_hz},
        {"interval_ms", interval_ms},
        {"localTime", getTimeNow()}
    }.dump());
    LOG_INFO("[PositionUpload] frequency updated: frequency_hz={}, interval_ms={}",
             frequency_hz, interval_ms);
}

void CommandExecutor::try_flush_msgs()
{
    if (!gateway_ || !gateway_->isWebConnected()) {
        return;
    }

    while (true)
    {
        std::string msg;
        {
            std::lock_guard<std::mutex> lock(msg_mutex_);
            if (pending_msgs_.empty()) {
                break;
            }
            msg = std::move(pending_msgs_.front());
            pending_msgs_.pop_front();
        }

        bool ok = gateway_->sendToWeb(msg);

        if (!ok)
        {
            std::lock_guard<std::mutex> lock(msg_mutex_);
            pending_msgs_.push_front(std::move(msg));
            break;
        }
    }
}

void CommandExecutor::launch_background_task(std::thread&& worker) {
    std::lock_guard<std::mutex> lock(worker_mutex_);
    background_workers_.emplace_back(std::move(worker));
}

void CommandExecutor::log_runtime_flags(const char* stage) const {
    pid_t nav_pid = -1;
    pid_t goal_pid = -1;
    pid_t mapping_pid = -1;
    pid_t cleanup_pid = -1;
    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        nav_pid = nav_process_;
        goal_pid = goal_process_;
        mapping_pid = mapping_process_;
        cleanup_pid = cleanup_process_;
    }

    LOG_INFO("[Runtime] stage={}, navigation_env_ready={}, localization_system_ready={}, localization_ready={}, upload_enable={}, nav_pid={}, goal_pid={}, mapping_pid={}, cleanup_pid={}",
             stage ? stage : "",
             navigation_env_ready_.load(std::memory_order_acquire),
             localization_system_ready_.load(std::memory_order_acquire),
             localization_ready_.load(std::memory_order_acquire),
             upload_enable_.load(std::memory_order_acquire),
             nav_pid,
             goal_pid,
             mapping_pid,
             cleanup_pid);
}

bool CommandExecutor::is_pid_still_running(pid_t pid) const {
    if (pid <= 0) {
        return false;
    }

    int status = 0;
    pid_t waited = waitpid(pid, &status, WNOHANG);
    if (waited == pid) {
        log_wait_status("navigation.sh", status);
        return false;
    }
    if (waited < 0 && errno != ECHILD) {
        LOG_WARN("[Process] waitpid check failed, pid={}, errno={}, error={}",
                 pid, errno, std::strerror(errno));
    }

    return kill(pid, 0) == 0 || errno == EPERM;
}

bool CommandExecutor::is_navigation_environment_running(std::string& reason) {
    pid_t nav_pid = -1;
    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        nav_pid = nav_process_;
    }

    if (is_pid_still_running(nav_pid)) {
        reason = "navigation process is still running, pid=" + std::to_string(nav_pid);
        return true;
    }

    if (nav_pid > 0) {
        std::lock_guard<std::mutex> lock(process_mutex_);
        if (nav_process_ == nav_pid) {
            nav_process_ = -1;
        }
    }

    if (navigation_env_ready_.load(std::memory_order_acquire) ||
        localization_ready_.load(std::memory_order_acquire)) {
        reason = "navigation environment state is active";
        return true;
    }

    std::string pids = trim_copy(exec_cmd(
        "pgrep -f '[f]ast_lio_localization localization_airy96.launch|"
        "[p]ct_planner global_planner.launch|"
        "[n]avActionServer.py|"
        "[g]lobalPlanner.py|"
        "[l]ocalPlanner.py|"
        "[p]ointcloud_tomography|"
        "[b]ase_link.py|"
        "[f]lat_neupan_map.py' 2>/dev/null || true"));
    if (!pids.empty()) {
        reason = "navigation environment process is already running, pids=" + pids;
        return true;
    }

    return false;
}

void CommandExecutor::log_wait_status(const std::string& name, int status) const {
    if (status == -1) {
        LOG_ERROR("[Process] {} wait/system failed, errno={}, error={}",
                  name, errno, std::strerror(errno));
        return;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code == 0) {
            LOG_INFO("[Process] {} exited normally, exit_code={}", name, exit_code);
        } else {
            LOG_ERROR("[Process] {} exited with error, exit_code={}", name, exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        LOG_ERROR("[Process] {} killed by signal={}", name, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        LOG_WARN("[Process] {} stopped by signal={}", name, WSTOPSIG(status));
    } else {
        LOG_WARN("[Process] {} ended with unknown status={}", name, status);
    }
}

bool CommandExecutor::log_file_access(const std::string& name, const std::string& path, int mode) const {
    errno = 0;
    if (access(path.c_str(), mode) == 0) {
        LOG_INFO("[FileCheck] {} ok, path={}, mode={}", name, path, mode);
        return true;
    }

    LOG_ERROR("[FileCheck] {} failed, path={}, mode={}, errno={}, error={}",
              name, path, mode, errno, std::strerror(errno));
    return false;
}

std::string CommandExecutor::shell_quote(const std::string& value) const {
    std::string quoted = "'";
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

std::string CommandExecutor::script_log_command(const std::string& script_path, const std::string& tag) const {
    const std::string log_path = LogManager::instance().get_log_file_path();
    return "set -o pipefail; "
           "bash " + shell_quote(script_path) + " 2>&1 | "
           "while IFS= read -r line; do "
           "printf '[%s] [info] [%s] [script:%s] %s\\n' "
           "\"$(date '+%Y-%m-%d %H:%M:%S.%3N')\" \"$$\" " + shell_quote(tag) + " \"$line\"; "
           "done >> " + shell_quote(log_path) + "; "
           "exit ${PIPESTATUS[0]}";
}

void CommandExecutor::ros_prepare(const json& payload_json) {
    (void)payload_json;
}

void CommandExecutor::loc_callback(const std_msgs::Int8::ConstPtr& msg) {
    bool ready = msg->data != 0;
    localization_system_ready_.store(ready, std::memory_order_release);
    if (msg->data != 0) {
        ROS_INFO("[Localization] localization system ready");
        LOG_INFO("[Localization] localization system ready, msg={}", static_cast<int>(msg->data));
    } else {
        localization_ready_.store(false, std::memory_order_release);
        upload_enable_.store(false, std::memory_order_release);
        ROS_WARN("[Localization] localization system waiting");
        LOG_WARN("[Localization] localization system waiting, msg={}", static_cast<int>(msg->data));
    }
}

bool CommandExecutor::is_managed_command(const std::string& cmd_type) const {
    return cmd_type == "initial_pose" ||
           cmd_type == "start_navigation" ||
           cmd_type == "navigation_task" ||
           cmd_type == "stop_navigation" ||
           cmd_type == "start_mapping" ||
           cmd_type == "stop_and_uploadmap" ||
           cmd_type == "nav-upload-successful" ||
           cmd_type == "start_task" ||
           cmd_type == "clear_waypoints" ||
           cmd_type == "cancel_task" ||
           cmd_type == "start_add_task" ||
           cmd_type == "start_add_point" ||
           cmd_type == "stop_add_point" ||
           cmd_type == "stop_add_task";
}

bool CommandExecutor::try_begin_command(const std::string& cmd_type) {
    if (!is_managed_command(cmd_type)) {
        return true;
    }

    std::lock_guard<std::mutex> lock(command_state_mutex_);
    if (!active_command_.empty()) {
        send_to_cloud(json{
            {"type", cmd_type},
            {"status", "busy"},
            {"code", 409},
            {"active_command", active_command_},
            {"localTime", getTimeNow()},
            {"message", "Another command is running, ignore duplicate operation"}
        }.dump());
        LOG_WARN("[CommandState] reject command={}, active={}", cmd_type, active_command_);
        return false;
    }

    active_command_ = cmd_type;
    LOG_INFO("[CommandState] begin command={}", cmd_type);
    return true;
}

void CommandExecutor::finish_command(const std::string& cmd_type) {
    if (!is_managed_command(cmd_type)) {
        return;
    }

    std::lock_guard<std::mutex> lock(command_state_mutex_);
    if (active_command_ == cmd_type) {
        LOG_INFO("[CommandState] finish command={}", cmd_type);
        active_command_.clear();
    } else if (!active_command_.empty()) {
        LOG_WARN("[CommandState] finish ignored, command={}, active={}", cmd_type, active_command_);
    }
}

void CommandExecutor::execute(const json& payload_json) {
    // ROS_INFO("    ======================= (handle_cloud_command) =======================    ");
    std::string cmd_type;
    try {
        json data = payload_json.is_string()
            ? json::parse(payload_json.get<std::string>())
            : payload_json;

        if (!data.is_object()) {
            LOG_WARN("[Command] ignore non-object payload: {}", data.dump());
            return;
        }

        auto value_to_string = [](const json& value) -> std::string {

            if (value.is_null()) {
                return "";
            }
            if (value.is_string()) {
                return value.get<std::string>();
            }
            return value.dump();
        };

        if (data.contains("type")) {
            cmd_type = value_to_string(data["type"]);
        }
        
        std::string target_device;
        if (data.contains("device_id")) {
            target_device = value_to_string(data["device_id"]);
        } else if (data.contains("robot_id")) {
            target_device = value_to_string(data["robot_id"]);
        }
        if (!target_device.empty() && target_device != device_id_) {
            std::cout << "不一样... " << target_device << " - " << device_id_ << std::endl;
            return;
        }
        
        LOG_INFO("[Command] received type={}, target_device={}, payload={}", cmd_type, target_device, data.dump());
        
        if (!try_begin_command(cmd_type)) {
            return;
        }

        if (cmd_type == "initial_pose") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_initial_pose(data);
        } else if (cmd_type == "start_navigation") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_start_navigation(data);
        } else if (cmd_type == "navigation_task") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_navigation_task(data);
        } else if (cmd_type == "set_dock_pose") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_set_dock_pose(data);
        } else if (cmd_type == "action_control" || cmd_type == "control") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            if (motion_handle_) {
                motion_handle_->handle_action_control(data);
            }
            LOG_INFO("{}", cmd_type);
        } else if (cmd_type == "set_position_upload_frequency") {
            handle_set_position_upload_frequency(data);
        } else if (cmd_type == "stop_navigation") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_stop_navigation(data);
            finish_command(cmd_type);
        } else if (cmd_type == "start_mapping") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_start_mapping(data);
            finish_command(cmd_type);
        } else if (cmd_type == "stop_and_uploadmap") {
            json stop_test = {
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            };
            send_to_cloud(stop_test.dump());
            LOG_INFO("{}", cmd_type);
            handle_stop_and_uploadmap(data);
        } else if (cmd_type == "nav-upload-successful") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            handle_start_sh(data);
            finish_command(cmd_type);
        } else if (cmd_type == "start_task") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            exec_rosservice_cmd("start_task", "/start_task");           
        } else if (cmd_type == "pause_task" || cmd_type == "resume_task") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            enqueue_task_state_request(cmd_type, cmd_type == "pause_task");
        } else if (cmd_type == "clear_waypoints") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            unsigned long long clear_attempt_id = 0;
            {
                std::lock_guard<std::mutex> lock(clear_waypoints_mutex_);
                clear_waypoints_pending_ = true;
                clear_attempt_id = ++clear_waypoints_attempt_id_;
            }
            launch_background_task(std::thread([this, clear_attempt_id]() {
                std::this_thread::sleep_for(std::chrono::seconds(kClearWaypointsAckTimeoutSec_));
                complete_clear_waypoints_result(
                    clear_attempt_id,
                    false,
                    "Timeout waiting for clear waypoints confirmation");
            }));
            exec_rosservice_cmd("clear_waypoints", "/clear_waypoints");           
        } else if (cmd_type == "cancel_task") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            exec_rosservice_cmd("cancel_task", "/cancel_task");           
        } else if (cmd_type == "start_recharge") { // 开始回充
           send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            exec_rosservice_cmd("start_recharge", "/start_recharge");  
        } else if (cmd_type == "stop_recharge") { // 结束回充
           send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            exec_rosservice_cmd("stop_recharge", "/stop_recharge");  
        } else if (cmd_type == "upload_require_file") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            LOG_INFO("{}", cmd_type);
            exec_upload_cmd(cmd_type);
        } else if (cmd_type == "hikvision_continuous") {
            if (hikvision_handle_ == nullptr) {
                send_to_cloud(json{
                    {"type", cmd_type},
                    {"status", "failed"},
                    {"error_message", "海康云台模块未初始化"}
                }.dump());
            } else {
                send_to_cloud(
                    hikvision_handle_->handle_continuous_command(data).dump());
            }
        } else if (cmd_type == "hikvision") {
            send_to_cloud(json{
                {"type", cmd_type},
                {"localTime", getTimeNow()},
                {"status", "received"}
            }.dump());
            if (hikvision_handle_ == nullptr) {
                send_to_cloud(json{
                    {"type", cmd_type},
                    {"error_message", "海康云台模块未初始化"}
                }.dump());
            } else {
                send_to_cloud(hikvision_handle_->handle_command(data).dump());
            }
        } else if (cmd_type == "set_param") {
            modify_handle_.updateParam(data);
        } else if (cmd_type == "bsy_test") {
            std::vector<std::string> bsy_file = {"666.pcd"};
            std::string bsy_file_path = "/home/jianmi/yq";
            std::string end_path = "yq/000";
            map_manager_->upload_map_sh(device_id_, "yq", "000", bsy_file_path, bsy_file, end_path, "6335d16b-ec5b-4183-99b7-36c075baeaa8");
        }
    } catch (const std::exception& e) {
        ROS_ERROR("Command parsing error: %s", e.what());

        try {
            ROS_ERROR_STREAM("Bad JSON:\n" << payload_json.dump(4));
        } catch (...) {}

        LOG_ERROR("[Command] parsing error: {}", e.what());
        if (cmd_type == "start_navigation" && robot_state_handle_) {
            robot_state_handle_->clear_navigation_environment();
        }
        finish_command(cmd_type);
    }
}

bool CommandExecutor::get_robot_position(double& x, double& y, double& z)
{
    LOG_INFO("获取机器狗的位置(get_robot_position) tf ... ");

    const std::string target_frame = "/map";
    const std::string source_frame = "/base_link";

    try
    {
        // 先检查 frame 是否存在
        if (!tf_listener_.frameExists(target_frame))
        {
            ROS_ERROR("[TF] target frame 不存在: %s", target_frame.c_str());
            LOG_ERROR("[TF] get_robot_position target frame missing: {}", target_frame);
            return false;
        }

        if (!tf_listener_.frameExists(source_frame))
        {
            ROS_ERROR("[TF] source frame 不存在: %s", source_frame.c_str());
            LOG_ERROR("[TF] get_robot_position source frame missing: {}", source_frame);
            return false;
        }

        // 检查 transform 是否可用
        if (!tf_listener_.canTransform(target_frame, source_frame, ros::Time(0)))
        {
            ROS_ERROR("[TF] 无法转换 %s -> %s", source_frame.c_str(), target_frame.c_str());

            std::string tf_error;
            tf_listener_.canTransform(target_frame, source_frame, ros::Time(0), &tf_error);

            ROS_ERROR("[TF] 详细原因: %s", tf_error.c_str());
            LOG_ERROR("[TF] get_robot_position cannot transform {} -> {}, reason={}",
                      source_frame, target_frame, tf_error);

            return false;
        }

        tf::StampedTransform transform;

        tf_listener_.lookupTransform(target_frame, source_frame, ros::Time(0), transform);

        x = transform.getOrigin().x();
        y = transform.getOrigin().y();
        z = transform.getOrigin().z();

        // ROS_INFO("[TF] 获取位置成功: x=%.3f y=%.3f z=%.3f", x, y, z);

        return true;
    } catch (tf::LookupException& ex)
    {
        ROS_ERROR("[TF] LookupException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_position LookupException: {}", ex.what());
    }
    catch (tf::ConnectivityException& ex)
    {
        ROS_ERROR("[TF] ConnectivityException: %s", ex.what()); // TF tree 断了
        LOG_ERROR("[TF] get_robot_position ConnectivityException: {}", ex.what());
    }
    catch (tf::ExtrapolationException& ex)
    {
        ROS_ERROR("[TF] ExtrapolationException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_position ExtrapolationException: {}", ex.what());
    }
    catch (tf::InvalidArgument& ex)
    {
        ROS_ERROR("[TF] InvalidArgument: %s", ex.what());
        LOG_ERROR("[TF] get_robot_position InvalidArgument: {}", ex.what());
    }
    catch (tf::TransformException& ex)
    {
        ROS_ERROR("[TF] TransformException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_position TransformException: {}", ex.what());
    }
    catch (std::exception& ex)
    {
        ROS_ERROR("[TF] std::exception: %s", ex.what());
        LOG_ERROR("[TF] get_robot_position std::exception: {}", ex.what());
    }

    return false;
}

bool CommandExecutor::get_robot_orientation(double& roll, double& pitch, double& yaw) {
    LOG_INFO("获取机器狗方向(get_robot_orientation)...");

    const std::string target_frame = "/map";
    const std::string source_frame = "/base_link";

    try
    {
        if (!tf_listener_.frameExists(target_frame))
        {
            ROS_ERROR("[TF] target frame 不存在: %s",
                      target_frame.c_str());
            LOG_ERROR("[TF] get_robot_orientation target frame missing: {}", target_frame);
            return false;
        }

        if (!tf_listener_.frameExists(source_frame))
        {
            ROS_ERROR("[TF] source frame 不存在: %s",
                      source_frame.c_str());
            LOG_ERROR("[TF] get_robot_orientation source frame missing: {}", source_frame);
            return false;
        }

        std::string tf_error;

        if (!tf_listener_.canTransform(target_frame, source_frame, ros::Time(0), &tf_error)) {
            ROS_ERROR("[TF] 无法转换 %s -> %s", source_frame.c_str(), target_frame.c_str());

            ROS_ERROR("[TF] 详细原因: %s", tf_error.c_str());
            LOG_ERROR("[TF] get_robot_orientation cannot transform {} -> {}, reason={}",
                      source_frame, target_frame, tf_error);

            return false;
        }

        tf::StampedTransform transform;

        tf_listener_.lookupTransform(target_frame, source_frame, ros::Time(0), transform);

        tf::Matrix3x3 m(transform.getRotation());

        m.getRPY(roll, pitch, yaw);

        // ROS_INFO("[TF] RPY: roll=%.3f pitch=%.3f yaw=%.3f", roll, pitch, yaw);

        return true;
    } catch (tf::LookupException& ex)
    {
        ROS_ERROR("[TF] LookupException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_orientation LookupException: {}", ex.what());
    }
    catch (tf::ConnectivityException& ex)
    {
        ROS_ERROR("[TF] ConnectivityException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_orientation ConnectivityException: {}", ex.what());
    }
    catch (tf::ExtrapolationException& ex)
    {
        ROS_ERROR("[TF] ExtrapolationException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_orientation ExtrapolationException: {}", ex.what());
    }
    catch (tf::TransformException& ex)
    {
        ROS_ERROR("[TF] TransformException: %s", ex.what());
        LOG_ERROR("[TF] get_robot_orientation TransformException: {}", ex.what());
    }

    return false;
}

/*
数据上报曾 (robot -> cloud)
全局路径上报 (upload_path), 当 ros 订阅主题的时候上报
{
  "type": "upload_path",
  "map_id": "map_001",
  "robot_id": "dog_001",
  "path_id": "path_1712345678",
  "timestamp": 1712345678,
  "frame_id": "map",
  "waypoints": [
    {
      "position": {
        "x": 1.2,
        "y": 3.4,
        "z": 0.0
      },
      "orientation": {
        "x": 0,
        "y": 0,
        "z": 0.7,
        "w": 0.7
      }
    }
  ]
}
*/

void CommandExecutor::global_path_callback(const nav_msgs::Path::ConstPtr& msg) {
    json waypoints;
    for (size_t i = 0; i < msg->poses.size(); i += 10) {
        const auto& pose = msg->poses[i].pose;
        waypoints.push_back({
            {"position", {
                {"x", pose.position.x},
                {"y", pose.position.y},
                {"z", pose.position.z}
            }},
            {"orientation", {
                {"x", pose.orientation.x},
                {"y", pose.orientation.y},
                {"z", pose.orientation.z},
                {"w", pose.orientation.w}
            }}
        });
    }
    
    const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json upload_msg = {
        {"type", "upload_path"},
        {"map_id", current_map_id_},
        {"device_id", device_id_},
        {"path_id", "path_" + std::to_string(timestamp_ms)},
        {"timestamp", timestamp_ms},
        {"frame_id", "map"},
        {"localTime", getTimeNow()},
        {"waypoints", waypoints}
    };
    
    // LOG_INFO("开始路径上报: {}", upload_msg.dump());
    send_to_cloud(upload_msg.dump());
}

// 实时位置上报 (upload_position), 收到 /localization 后上报
/*
{
  "type": "upload_position",
  "robot_id": "dog_001",
  "timestamp": 1712345678123,
  "position": {
    "x": 1.2,
    "y": 3.4,
    "z": 0
  },
  "orientation": {
    "x": 0,
    "y": 0,
    "z": 0.707,
    "w": 0.707
  }
}
*/
void CommandExecutor::localization_callback(const nav_msgs::Odometry::ConstPtr& msg) {
    if (!msg) {
        return;
    }

    const auto& pose = msg->pose.pose;
    const auto& position = pose.position;
    const auto& orientation = pose.orientation;

    localization_ready_.store(true, std::memory_order_release);

    if (!should_send_upload_position()) {
        return;
    }

    const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json pos_msg = {
        {"type", "upload_position"},
        {"device_id", device_id_},
        {"timestamp", timestamp_ms},
        {"position", {{"x", position.x}, {"y", position.y}, {"z", position.z}}},
        {"localTime", getTimeNow()},
        {"orientation", {{"x", orientation.x}, {"y", orientation.y}, {"z", orientation.z}, {"w", orientation.w}}}
    };

    send_to_cloud(pos_msg.dump());
}

void CommandExecutor::initial_pose_rosout_callback(const rosgraph_msgs::Log::ConstPtr& msg) {
    if (!msg) {
        return;
    }

    auto send_recharge_status = [this](const std::string& stage,
                                       const std::string& event,
                                       const std::string& level,
                                       const std::string& status,
                                       const std::string& message) {
        json status_msg = {
            {"type", "recharge_status"},
            {"timestamp", static_cast<int>(time(nullptr))},
            {"device_id", device_id_},
            {"stage", stage},
            {"event", event},
            {"level", level},
            {"localTime", getTimeNow()},
            {"status", status},
            {"message", message}
        };
        send_to_cloud(status_msg.dump());
        LOG_INFO("[RechargeStatus] stage={}, event={}, status={}, message={}",
                 stage, event, status, message);
    };

    auto send_navigation_task_state = [this](
        const std::string& event,
        const std::string& status,
        const std::string& message,
        const std::string& source_message) {
        json state_msg = {
            {"type", "navigation_task_state"},
            {"timestamp", unix_timestamp_ms()},
            {"device_id", device_id_},
            {"code", 0},
            {"event", event},
            {"status", status},
            {"message", message},
            {"localTime", getTimeNow()},
            {"source_message", source_message}
        };
        send_to_cloud(state_msg.dump());
        LOG_INFO("[NavigationTaskState] event={}, status={}, source_message={}",
                 event, status, source_message);
    };

    const std::string& rosout_message = msg->msg;
    int completed_round = 0;
    if (parse_completed_task_round(rosout_message, completed_round)) {
        json round_msg = {
            {"type", "navigation_task_state"},
            {"timestamp", static_cast<int>(time(nullptr))},
            {"device_id", device_id_},
            {"code", 0},
            {"event", "task_round_completed"},
            {"status", "success"},
            {"round", completed_round},
            {"message", rosout_message},
            {"localTime", getTimeNow()},
            {"source_message", rosout_message}
        };
        send_to_cloud(round_msg.dump());
        LOG_INFO("[NavigationTaskState] event=task_round_completed, round={}, source_message={}",
                 completed_round, rosout_message);
    }

    if (rosout_message.find("任务暂停") != std::string::npos &&
        rosout_message.find("停止局部控制器") != std::string::npos) {
        {
            std::lock_guard<std::mutex> lock(task_state_mutex_);
            task_paused_ = true;
        }
        send_navigation_task_state(
            "task_paused", "paused", "任务已暂停", rosout_message);
        return;
    }
    if (rosout_message.find("检测到从暂停中恢复") != std::string::npos) {
        {
            std::lock_guard<std::mutex> lock(task_state_mutex_);
            task_paused_ = false;
        }
        send_navigation_task_state(
            "task_resumed",
            "running",
            "任务已恢复，继续执行导航任务",
            rosout_message);
        return;
    }

    static const std::vector<std::string> waypoint_markers = {
        "到达路径点",
        "到达任务点",
        "达到路径点",
        "达到任务点"
    };
    for (const std::string& marker : waypoint_markers) {
        const size_t marker_pos = rosout_message.find(marker);
        if (marker_pos == std::string::npos) {
            continue;
        }

        const size_t number_pos = rosout_message.find_first_of(
            "0123456789", marker_pos + marker.size());
        if (number_pos == std::string::npos) {
            LOG_WARN("[NavigationWaypoint] missing waypoint index, message={}",
                     rosout_message);
            break;
        }

        char* number_end = nullptr;
        errno = 0;
        const long long waypoint_index = std::strtoll(
            rosout_message.c_str() + number_pos, &number_end, 10);
        if (errno != 0 || number_end == rosout_message.c_str() + number_pos ||
            waypoint_index <= 0) {
            LOG_WARN("[NavigationWaypoint] invalid waypoint index, message={}",
                     rosout_message);
            break;
        }

        size_t reached_points = 0;
        size_t total_points = 0;
        {
            std::lock_guard<std::mutex> lock(task_execution_progress_mutex_);
            total_points = task_execution_total_points_;
            reached_points = static_cast<size_t>(waypoint_index);
            if (total_points > 0) {
                reached_points = std::min(reached_points, total_points);
                task_execution_reached_points_ = std::max(
                    task_execution_reached_points_, reached_points);
                reached_points = task_execution_reached_points_;
                task_execution_active_ = reached_points < total_points;
            } else {
                task_execution_reached_points_ = std::max(
                    task_execution_reached_points_, reached_points);
                reached_points = task_execution_reached_points_;
            }
        }

        json waypoint_msg = {
            {"type", "navigation_waypoint_reached"},
            {"timestamp", unix_timestamp_ms()},
            {"code", 0},
            {"device_id", device_id_},
            {"waypoint_index", waypoint_index},
            {"status", "reached"},
            {"localTime", getTimeNow()},
            {"message", rosout_message}
        };
        if (total_points > 0) {
            const std::string progress = fmt::format("{}/{}", reached_points, total_points);
            waypoint_msg["reached_points"] = reached_points;
            waypoint_msg["total_points"] = total_points;
            waypoint_msg["progress"] = progress;
            waypoint_msg["progress_message"] = fmt::format("导航任务点已经完成了 {}", progress);
        }
        send_to_cloud(waypoint_msg.dump());
        LOG_INFO("[NavigationWaypoint] reached index={}, progress={}/{}, message={}",
                 waypoint_index, reached_points, total_points, rosout_message);
        return;
    }

    if (rosout_message.find("任务中断！进入回充逻辑...") != std::string::npos) {
        send_recharge_status("interrupt", "task_interrupted", "warn", "interrupted", rosout_message);
        return;
    }
    if (rosout_message.find("等待充电桩位姿话题 (/dock_pose)...") != std::string::npos) {
        send_recharge_status("wait_dock_pose", "waiting_dock_pose", "info", "waiting", rosout_message);
        return;
    }
    if (rosout_message.find("正在前往充电桩...") != std::string::npos) {
        send_recharge_status("go_to_dock", "going_to_dock", "info", "running", rosout_message);
        return;
    }
    if (rosout_message.find("已成功到达充电桩。") != std::string::npos) {
        send_recharge_status("go_to_dock", "arrived_dock", "info", "success", rosout_message);
        return;
    }
    if (rosout_message.find("移动前往充电桩失败，请重新下发坐标或检查障碍物。") != std::string::npos) {
        send_recharge_status("go_to_dock", "move_to_dock_failed", "error", "failed", rosout_message);
        return;
    }
    if (rosout_message.find("前往充电桩路径规划失败:") != std::string::npos) {
        send_recharge_status("go_to_dock", "plan_to_dock_failed", "error", "failed", rosout_message);
        return;
    }
    if (rosout_message.find("请下发一个新的、有效的充电点坐标。") != std::string::npos) {
        send_recharge_status("go_to_dock", "need_new_dock_pose", "error", "failed", rosout_message);
        return;
    }
    if (rosout_message.find("正在充电中... 请在任务完成后点击'结束回充'") != std::string::npos) {
        send_recharge_status("charging", "charging", "info", "charging", rosout_message);
        return;
    }
    if (rosout_message.find("回充结束，正在返回任务中断点...") != std::string::npos) {
        send_recharge_status("return_to_task", "returning_to_interrupt_point", "info", "running", rosout_message);
        return;
    }
    if (rosout_message.find("已回到中断点，继续执行后续航点。") != std::string::npos) {
        send_recharge_status("come_to_back", "come_to_back_success", "info", "success", rosout_message);
        return;
    }
    if (rosout_message.find("无法规划返回任务中断点的路径！请手动处理。") != std::string::npos) {
        send_recharge_status("return_to_task", "return_plan_failed", "error", "failed", rosout_message);
        return;
    }

    handle_navigation_planning_rosout(msg->msg);

    if (msg->msg.find("Tomogram exported:") != std::string::npos ||
        msg->msg.find("缓存地图发布完成") != std::string::npos) {
        unsigned long long attempt_id = 0;
        {
            std::lock_guard<std::mutex> lock(navigation_env_mutex_);
            if (!navigation_env_pending_) {
                return;
            }
            attempt_id = navigation_env_attempt_id_;
        }
        complete_navigation_env_ready(attempt_id, true, msg->msg);
        return;
    }

    if (msg->msg.find("通过话题添加航点") != std::string::npos) {
        unsigned long long attempt_id = 0;
        bool complete = false;
        size_t confirmed = 0;
        size_t expected = 0;
        {
            std::lock_guard<std::mutex> lock(navigation_task_mutex_);
            if (!navigation_task_pending_) {
                return;
            }
            attempt_id = navigation_task_attempt_id_;
            ++navigation_task_confirmed_points_;
            confirmed = navigation_task_confirmed_points_;
            expected = navigation_task_expected_points_;
            complete = expected > 0 && confirmed >= expected;
        }

        LOG_INFO("[NavigationTask] waypoint confirmed by rosout, confirmed={}, expected={}, message={}",
                 confirmed, expected, msg->msg);
        if (complete) {
            complete_navigation_task_result(attempt_id, true, "下发导航任务点成功！！！");
        }
        return;
    }

    if (msg->msg.find("所有航点已清除，循环次数已重置为 1") != std::string::npos ||
        msg->msg.find("所有航点已清除") != std::string::npos) {
        unsigned long long attempt_id = 0;
        {
            std::lock_guard<std::mutex> lock(clear_waypoints_mutex_);
            if (!clear_waypoints_pending_) {
                return;
            }
            attempt_id = clear_waypoints_attempt_id_;
        }
        complete_clear_waypoints_result(attempt_id, true, msg->msg);
        return;
    }

    if (msg->msg.find("Initialize successfully!!!!!!") == std::string::npos) {
        return;
    }

    unsigned long long attempt_id = 0;
    {
        std::lock_guard<std::mutex> lock(initial_pose_mutex_);
        if (!initial_pose_pending_) {
            return;
        }
        attempt_id = initial_pose_attempt_id_;
    }

    complete_initial_pose_result(attempt_id, true, "Initialize successfully!!!!!!");
}

void CommandExecutor::handle_navigation_planning_rosout(const std::string& message) {
    if (message.find("【导航预规划失败】") != std::string::npos) {
        std::lock_guard<std::mutex> lock(navigation_planning_failure_mutex_);
        navigation_planning_failure_pending_ = true;
        navigation_planning_fault_position_.clear();
        navigation_planning_failure_reason_.clear();
        navigation_planning_target_valid_ = false;
        navigation_planning_target_x_ = 0.0;
        navigation_planning_target_y_ = 0.0;
        navigation_planning_target_z_ = 0.0;
        return;
    }

    if (message.find("故障位置:") != std::string::npos) {
        std::lock_guard<std::mutex> lock(navigation_planning_failure_mutex_);
        navigation_planning_failure_pending_ = true;
        navigation_planning_fault_position_ = text_after_marker(message, "故障位置:");
        return;
    }

    if (message.find("目标坐标:") != std::string::npos) {
        const std::string coords = text_after_marker(message, "目标坐标:");
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        const int matched = std::sscanf(coords.c_str(), "x=%lf, y=%lf, z=%lf", &x, &y, &z);

        std::lock_guard<std::mutex> lock(navigation_planning_failure_mutex_);
        navigation_planning_failure_pending_ = true;
        if (matched == 3) {
            navigation_planning_target_x_ = x;
            navigation_planning_target_y_ = y;
            navigation_planning_target_z_ = z;
            navigation_planning_target_valid_ = true;
        }
        return;
    }

    if (message.find("失败原因:") != std::string::npos) {
        std::lock_guard<std::mutex> lock(navigation_planning_failure_mutex_);
        navigation_planning_failure_pending_ = true;
        navigation_planning_failure_reason_ = text_after_marker(message, "失败原因:");
        return;
    }

    if (message.find("任务结束: FAILURE_PLANNING") != std::string::npos ||
        message.find("任务结束: FAILURE_PLANNING") != std::string::npos) {
        send_navigation_planning_failure();
    }
}

void CommandExecutor::send_navigation_planning_failure() {
    std::string fault_position;
    std::string failure_reason;
    double target_x = 0.0;
    double target_y = 0.0;
    double target_z = 0.0;
    bool target_valid = false;

    {
        std::lock_guard<std::mutex> lock(navigation_planning_failure_mutex_);
        if (!navigation_planning_failure_pending_) {
            return;
        }

        fault_position = navigation_planning_fault_position_;
        failure_reason = navigation_planning_failure_reason_;
        target_x = navigation_planning_target_x_;
        target_y = navigation_planning_target_y_;
        target_z = navigation_planning_target_z_;
        target_valid = navigation_planning_target_valid_;

        navigation_planning_failure_pending_ = false;
        navigation_planning_fault_position_.clear();
        navigation_planning_failure_reason_.clear();
        navigation_planning_target_valid_ = false;
    }

    if (fault_position.empty()) {
        fault_position = "未知目标点";
    }
    if (failure_reason.empty()) {
        failure_reason = "全局规划器无法找到路径，请检查目标点是否在地图外、悬空或在障碍物内。";
    }

    json result_msg = {
        {"type", "navigation_planning_failed"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"device_id", device_id_},
        {"code", 1},
        {"status", "failed"},
        {"fault_position", fault_position},
        {"reason", failure_reason},
        {"localTime", getTimeNow()},
        {"message", "导航预规划失败"}
    };

    if (target_valid) {
        result_msg["target"] = {{"x", target_x}, {"y", target_y}, {"z", target_z}};
    }

    send_to_cloud(result_msg.dump());
    LOG_WARN("[NavigationPlanning] failed, fault_position={}, reason={}",
             fault_position, failure_reason);
}

void CommandExecutor::complete_initial_pose_result(unsigned long long attempt_id, bool success, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(initial_pose_mutex_);
        if (!initial_pose_pending_ || attempt_id != initial_pose_attempt_id_) {
            return;
        }
        initial_pose_pending_ = false;
    }

    upload_enable_.store(success, std::memory_order_release);
    localization_ready_.store(success, std::memory_order_release);

    json result_msg = {
        {"type", "ack_initial_pose"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"device_id", device_id_},
        {"code", success ? 0 : 1},
        {"status", success ? "success" : "failed"},
        {"localTime", getTimeNow()},
        {"message", message}
    };
    send_to_cloud(result_msg.dump());

    if (success) {
        ROS_INFO("Initial pose succeeded: %s", message.c_str());
        LOG_INFO("[InitialPose] success, message={}", message);
    } else {
        ROS_WARN("Initial pose failed: %s", message.c_str());
        LOG_WARN("[InitialPose] failed, message={}", message);
    }
    finish_command("initial_pose");
}

void CommandExecutor::complete_navigation_env_ready(unsigned long long attempt_id, bool success, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(navigation_env_mutex_);
        if (!navigation_env_pending_ || attempt_id != navigation_env_attempt_id_) {
            return;
        }
        navigation_env_pending_ = false;
    }

    navigation_env_ready_.store(success, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);
    upload_enable_.store(false, std::memory_order_release);
    if (robot_state_handle_) {
        if (success) {
            robot_state_handle_->set_navigation_env_active(true);
        } else {
            robot_state_handle_->clear_navigation_environment();
        }
    }

    json ready_msg = {
        {"type", "initial_pose_ready"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"device_id", device_id_},
        {"code", success ? 0 : -1},
        {"status", success ? "ready" : "failed"},
        {"localTime", getTimeNow()},
        {"message", "重定位已准备就绪！！！"}
    };
    send_to_cloud(ready_msg.dump());

    if (success) {
        ROS_INFO("Initial pose can be sent: %s", message.c_str());
        LOG_INFO("[NavigationEnv] initial pose ready, message={}", message);
    } else {
        ROS_WARN("Initial pose is not ready: %s", message.c_str());
        LOG_WARN("[NavigationEnv] initial pose not ready, message={}", message);
    }
    finish_command("start_navigation");
}

void CommandExecutor::complete_navigation_task_result(unsigned long long attempt_id, bool success, const std::string& message) {
    size_t confirmed = 0;
    size_t expected = 0;
    {
        std::lock_guard<std::mutex> lock(navigation_task_mutex_);
        if (!navigation_task_pending_ || attempt_id != navigation_task_attempt_id_) {
            return;
        }
        navigation_task_pending_ = false;
        confirmed = navigation_task_confirmed_points_;
        expected = navigation_task_expected_points_;
    }

    json result_msg = {
        {"type", "ack_navigation_task"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"device_id", device_id_},
        {"code", success ? 0 : -1},
        {"status", success ? "success" : "failed"},
        {"confirmed_points", confirmed},
        {"localTime", getTimeNow()},
        {"expected_points", expected},
        {"message", message}
    };
    send_to_cloud(result_msg.dump());

    if (success) {
        ROS_INFO("Navigation task waypoints succeeded: %zu/%zu", confirmed, expected);
        LOG_INFO("[NavigationTask] success, confirmed={}, expected={}, message={}",
                 confirmed, expected, message);
    } else {
        ROS_WARN("Navigation task waypoints failed: %zu/%zu, %s", confirmed, expected, message.c_str());
        LOG_WARN("[NavigationTask] failed, confirmed={}, expected={}, message={}",
                 confirmed, expected, message);
    }
    finish_command("navigation_task");
}

void CommandExecutor::complete_clear_waypoints_result(unsigned long long attempt_id, bool success, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(clear_waypoints_mutex_);
        if (!clear_waypoints_pending_ || attempt_id != clear_waypoints_attempt_id_) {
            return;
        }
        clear_waypoints_pending_ = false;
    }

    json result_msg = {
        {"type", "ack_clear_waypoints"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"device_id", device_id_},
        {"code", success ? 0 : 1},
        {"status", success ? "success" : "failed"},
        {"localTime", getTimeNow()},
        {"message", message}
    };
    send_to_cloud(result_msg.dump());

    if (success) {
        ROS_INFO("Clear waypoints succeeded: %s", message.c_str());
        LOG_INFO("[ClearWaypoints] success, message={}", message);
    } else {
        ROS_WARN("Clear waypoints failed: %s", message.c_str());
        LOG_WARN("[ClearWaypoints] failed, message={}", message);
    }
    finish_command("clear_waypoints");
    finish_command("stop_add_point");
}

void CommandExecutor::telemetry_loop() {
    ros::Rate rate(1);
    while (ros::ok()) {
        if (upload_enable_.load(std::memory_order_acquire)) {
            double x, y, z;
            if (get_robot_position(x, y, z) && should_send_upload_position()) {
                double roll, pitch, yaw;
                get_robot_orientation(roll, pitch, yaw);
                
                const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                json pos_msg = {
                    {"type", "upload_position"},
                    {"device_id", device_id_},
                    {"timestamp", timestamp_ms},
                    {"localTime", getTimeNow()},
                    {"position", {{"x", x}, {"y", y}, {"z", z}}},
                    {"orientation", {{"x", 0}, {"y", 0}, {"z", sin(yaw/2)}, {"w", cos(yaw/2)}}}
                };
                send_to_cloud(pos_msg.dump());
            }
        }
        rate.sleep();
    }
}

void CommandExecutor::handle_initial_pose(const json& data)
{
    ROS_INFO("========== 开始重定位(initial_pose) ==========");
    unsigned long long attempt_id = 0;
    {
        std::lock_guard<std::mutex> lock(initial_pose_mutex_);
        initial_pose_pending_ = true;
        attempt_id = ++initial_pose_attempt_id_;
    }
    upload_enable_.store(false, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);

    auto pos = data["pose"]["position"];
    auto ori = data["pose"]["orientation"];

    double x = pos.value("x", 0.0);
    double y = pos.value("y", 0.0);
    double z = pos.value("z", 0.0);

    double qx = ori.value("x", 0.0);
    double qy = ori.value("y", 0.0);
    double qz = ori.value("z", 0.0);
    double qw = ori.value("w", 1.0);

    tf::Quaternion q(qx, qy, qz, qw);

    double roll, pitch, yaw;

    tf::Matrix3x3(q).getRPY(roll, pitch, yaw);

    ROS_INFO("x = %.3f", x);
    ROS_INFO("y = %.3f", y);
    ROS_INFO("z = %.3f", z);
    ROS_INFO("yaw = %.3f", yaw);

    std::string final_cmd = fmt::format(
        R"(gnome-terminal -- bash -c "
            source /opt/ros/noetic/setup.bash &&
            source /home/jianmi/fast_wa/devel/setup.bash &&
            rosrun fast_lio_localization publish_initial_pose.py {} {} {} {} {} {};
            echo 'Terminal will close in 10 seconds...';
            sleep 10"
        )",
    x, y, z, yaw, pitch, roll);

    ROS_INFO("Execute CMD:");
    ROS_INFO("%s", final_cmd.c_str());

    std::thread([final_cmd]() {

        int ret = std::system(final_cmd.c_str());

        ROS_INFO("Terminal return: %d", ret);

    }).detach();

    ROS_INFO("Initial Pose sent.");
    LOG_INFO("[InitialPose] initial pose sent, waiting for localization result, attempt_id={}", attempt_id);

    launch_background_task(std::thread([this, attempt_id]() {
        std::this_thread::sleep_for(std::chrono::seconds(kInitialPoseTimeoutSec_));
        complete_initial_pose_result(
            attempt_id,
            false,
            "Timeout waiting for Initialize successfully!!!!!!");
    }));
}

// 2、开启导航 (环境初始化)
void CommandExecutor::handle_start_navigation(const json& data) {

    std::string running_reason;
    if (is_navigation_environment_running(running_reason)) {
        json msg = {
            {"type", "ack_start_navigation"},
            {"timestamp", static_cast<int>(time(nullptr))},
            {"device_id", device_id_},
            {"code", -1},
            {"status", "Navigation environment already running"},
            {"localTime", getTimeNow()},
            {"message", running_reason}
        };
        send_to_cloud(msg.dump());
        LOG_WARN("[NavigationEnv] reject duplicate start_navigation: {}", running_reason);
        finish_command("start_navigation");
        return;
    }

    std::string display_name = data.value("display_name", "Unnamed Map");
    std::string address = data.value("address", "");
    std::string uuid_old = data.value("uuid", "");
    if (robot_state_handle_) {
        robot_state_handle_->update_navigation_environment(false, address, display_name);
    }
    std::string uuid = get_uuid(address, display_name, http_ip_, http_port_);
    std::cout << "uuid_old: " << uuid_old << " ----> " << "uuid_new: " << uuid << std::endl; 
    if (uuid.empty()) {
        json msg = {
            {"type", "ack_start_navigation"},
            {"timestamp", static_cast<int>(time(nullptr))},
            {"device_id", device_id_},
            {"code", 4},
            {"status", "get uuid failed"},
            {"address", address},
            {"localTime", getTimeNow()},
            {"display_name", display_name}
        };
        send_to_cloud(msg.dump());
        LOG_ERROR("[NavigationEnv] get uuid failed, address={}, display_name={}", address, display_name);
        if (robot_state_handle_) {
            robot_state_handle_->clear_navigation_environment();
        }
        finish_command("start_navigation");
        return;
    }

    // 3步文件操作
    //(1) 删除相应的文件
    fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/navmap.pcd");
    fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomogram/navmap.pickle");

    //(2) 先进行拷贝到 g_nav_map 文件夹下
    fs::path file_path = fs::path("/home/jianmi/map_manager_local") / uuid;
    fs::copy(
        file_path / "navmap.pcd",
        "/home/jianmi/g_nav_map/navmap.pcd",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        file_path / "navmap.pickle",
        "/home/jianmi/g_nav_map/navmap.pickle",
        fs::copy_options::overwrite_existing
    );   

    //(3) 在从 g_nav_map 文件夹下拷贝到相应的路径下
    fs::copy(
        "/home/jianmi/g_nav_map/navmap.pcd",
        "/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/navmap.pcd",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        "/home/jianmi/g_nav_map/navmap.pickle",
        "/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomogram/navmap.pickle",
        fs::copy_options::overwrite_existing
    );

    ROS_INFO("    ======================= 开始导航 (handle_start_navigation) =======================    ");
    LOG_INFO(" 开启导航(handle_start_navigation) ... ");
    LOG_INFO("[NavigationEnv] start_navigation begin, payload={}", data.dump());
    log_runtime_flags("start_navigation_begin");
    navigation_env_ready_.store(false, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);
    upload_enable_.store(false, std::memory_order_release);
    
    // 校验参数，如果没有传地址或地图名，可以报错或使用默认值
    if (address.empty() || display_name.empty()) {
        ROS_ERROR("Start navigation failed: 'address' or 'display_name' is missing.");
        LOG_ERROR("[NavigationEnv] missing map information, address={}, display_name={}",
                  address, display_name);

        json msg1 = {
            {"type", "ack_start_navigation"},
            {"device_id", device_id_},
            {"localTime", getTimeNow()},
            {"code", -2},
            {"status", "Missing map information"}
        };
        send_to_cloud(msg1.dump());
        LOG_INFO("开始路径上报: {}", msg1.dump());
        if (robot_state_handle_) {
            robot_state_handle_->clear_navigation_environment();
        }
        finish_command("start_navigation");
        return;
    }
    
    ROS_INFO("Preparing map: %s -> %s...", address.c_str(), display_name.c_str());

    launch_background_task(
        std::thread(
            &CommandExecutor::start_navigation_async,
            this,
            data,
            uuid_old,
            uuid
        )
    );
}

void CommandExecutor::start_navigation_async(json data, std::string uuid_old, std::string uuid) {
    std::string address = data.value("address", "");
    std::string display_name = data.value("display_name", "");
    LOG_INFO("[NavigationEnv] async prepare map begin, address={}, display_name={}, map_dir={}",
             address, display_name, MAP_MANAGER_LOCAL);

    std::cout << "旧的 uuid " << uuid_old << "   新的 uuid " << uuid << std::endl;
    std::vector<std::string> download_map_files = {"gloabl_map.pcd", "navmap.pcd", "lightweight_map.npy", "navmap.pickle"};
    bool success = true;
    fs::path file_path = fs::path(MAP_MANAGER_LOCAL) / uuid;
    if (uuid_old != uuid)
    {
        if (map_manager_)
        {
            success = map_manager_->download_and_deploy(
                address,
                display_name,
                download_map_files,
                file_path
            );
        }
        else
        {
            success = false;
            LOG_ERROR("[NavigationEnv] map_manager ptr is null, cannot download map");
        }
    }

    std::cout << "map download result: " << (success ? 1 : 0) << std::endl;

    if (!success) {
        ROS_ERROR("Map deployment failed for %s. Navigation aborted.", display_name.c_str());
        LOG_ERROR("[NavigationEnv] map deployment failed, address={}, display_name={}, map_dir={}",
                  address, display_name, map_dir_);
        navigation_env_ready_.store(false, std::memory_order_release);
        localization_ready_.store(false, std::memory_order_release);
        upload_enable_.store(false, std::memory_order_release);
        if (robot_state_handle_) {
            robot_state_handle_->clear_navigation_environment();
        }
        log_runtime_flags("start_navigation_map_deploy_failed");

        json msg2 = {
            {"type", "ack_start_navigation"},
            {"timestamp", (int)time(nullptr)},
            {"device_id", device_id_},
            {"localTime", getTimeNow()},
            {"code", -1},
            {"status", "Map preparation failed"}
        };
        send_to_cloud(msg2.dump());
        LOG_INFO("开始路径上报: {}", msg2.dump());
        finish_command("start_navigation");
        return;
    }

    ROS_INFO("Map ready. Starting navigation environment scripts...");
    LOG_INFO("[NavigationEnv] map ready, starting scripts");

    std::string nav_script = scripts_path_ + "/navigation.sh";
    std::string nav_command = fmt::format(
    R"(gnome-terminal -- bash -c "set -e;
        source /opt/ros/noetic/setup.bash &&
        source /home/jianmi/catkin_ws/devel/setup.bash &&
        source /home/jianmi/catkin_livox/devel/setup.bash --extend &&
        source /home/jianmi/catkin_pct/devel/setup.bash --extend &&
        source /home/jianmi/fast_wa/devel/setup.bash --extend &&
        export LD_LIBRARY_PATH=/home/jianmi/PCT_planner/planner/lib/3rdparty/gtsam-4.1.1/install/lib:$LD_LIBRARY_PATH &&
        export LD_LIBRARY_PATH=/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/lib/3rdparty/gtsam-4.1.1/install/lib:$LD_LIBRARY_PATH &&
        bash {} ;
        exec bash"
        )",nav_script
    );

    LOG_INFO("[NavigationEnv] script paths: nav_script={}", nav_script);

    pid_t nav_pid = fork();
    if (nav_pid < 0) {
        ROS_ERROR("Fork failed for navigation script");
        LOG_ERROR("[NavigationEnv] fork failed for navigation.sh, errno={}, error={}",
                  errno, std::strerror(errno));
        navigation_env_ready_.store(false, std::memory_order_release);
        localization_ready_.store(false, std::memory_order_release);
        upload_enable_.store(false, std::memory_order_release);
        if (robot_state_handle_) {
            robot_state_handle_->clear_navigation_environment();
        }
        log_runtime_flags("start_navigation_nav_fork_failed");

        json msg4 = {
            {"type", "ack_start_navigation"},
            {"timestamp", (int)time(nullptr)},
            {"device_id", device_id_},
            {"code", -2},
            {"localTime", getTimeNow()},
            {"status", "Script Error: fork navigation failed"}
        };
        send_to_cloud(msg4.dump());
        LOG_INFO("开始路径上报: {}", msg4.dump());
        finish_command("start_navigation");
        return;
    }

    if (nav_pid == 0) {
        setpgid(0, 0);
        execl("/bin/bash", "bash", "-c", nav_command.c_str(), (char*)NULL);
        std::fprintf(stderr, "[NavigationEnv] execl failed for navigation.sh, path=%s, errno=%d, error=%s\n",
                     nav_script.c_str(), errno, std::strerror(errno));
        perror("execl nav failed");
        exit(1);
    }
    LOG_INFO("[NavigationEnv] navigation.sh started, pid={}", nav_pid);

    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        nav_process_ = nav_pid;
    }
    if (robot_state_handle_) {
        robot_state_handle_->set_navigation_env_active(true);
    }

    unsigned long long nav_env_attempt_id = 0;
    {
        std::lock_guard<std::mutex> lock(navigation_env_mutex_);
        navigation_env_pending_ = true;
        nav_env_attempt_id = ++navigation_env_attempt_id_;
    }

    launch_background_task(std::thread([this, nav_env_attempt_id]() {
        std::this_thread::sleep_for(std::chrono::seconds(kNavigationEnvReadyTimeoutSec_));
        complete_navigation_env_ready(
            nav_env_attempt_id,
            false,
            "Timeout waiting for Tomogram exported");
    }));

    sleep(5);

    navigation_env_ready_.store(false, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);
    upload_enable_.store(false, std::memory_order_release);
    ROS_INFO("Navigation environment started. Waiting for tomogram export before initial_pose.");
    LOG_INFO("[NavigationEnv] environment started, waiting for Tomogram exported, attempt_id={}", nav_env_attempt_id);
    log_runtime_flags("start_navigation_started");

    json msg3 = {
        {"type", "ack_start_navigation"},
        {"timestamp", (int)time(nullptr)},
        {"localTime", getTimeNow()},
        {"device_id", device_id_},
        {"code", 0},
        {"status", "Map " + display_name + " deployed and environment started, waiting for tomogram export"}
    };
    send_to_cloud(msg3.dump());
    std::cout << "开启导航环境以全部准备就绪... " << std::endl;
    LOG_INFO("开始路径上报: {}", msg3.dump());
}

void CommandExecutor::handle_navigation_task(const json& data) {
    std::cout << "start handle_navigation_task... " << std::endl;
    pct_planner::MultiWaypointNavigationGoal goal;

    const json &points = data.value("target_points", json::array());
    if (points.empty())
    {
        ROS_WARN("[CommandExecutor] navigation_task 没有有效目标点，取消发送");
        return;
    }

    unsigned long long nav_task_attempt_id = 0;
    {
        std::lock_guard<std::mutex> lock(navigation_task_mutex_);
        navigation_task_pending_ = true;
        nav_task_attempt_id = ++navigation_task_attempt_id_;
        navigation_task_expected_points_ = points.size();
        navigation_task_confirmed_points_ = 0;
    }

    launch_background_task(std::thread([this, nav_task_attempt_id]() {
        std::this_thread::sleep_for(std::chrono::seconds(kNavigationTaskAckTimeoutSec_));
        complete_navigation_task_result(
            nav_task_attempt_id,
            false,
            "下发导航任务点失败！！！");
    }));

    int cycles = data.value("times", 1) < 1 ? 1 : data.value("times", 1);
    std_msgs::Int32 msg;
    msg.data = cycles;

    cycles_pub_.publish(msg);

    ros::Duration(0.05).sleep(); 

    for (const auto &pt : points)
    {
        geometry_msgs::PoseStamped target;

        // target.header.stamp = ros::Time::now();
        target.header.stamp = ros::Time(pt.value("wait_time", 0.0));
        target.header.frame_id = data.value("frame_id", "map");

        target.pose.position.x = pt.value("x", 0.0);
        target.pose.position.y = pt.value("y", 0.0);
        target.pose.position.z = pt.value("z", 0.0);

        target.pose.orientation.x = pt["orientation"].value("x", 0.0);
        target.pose.orientation.y = pt["orientation"].value("y", 0.0);
        target.pose.orientation.z = pt["orientation"].value("z", 0.0);
        target.pose.orientation.w = pt["orientation"].value("w", 1.0);

        goal_pub_.publish(target);

        goal.waypoints.push_back(target);
    }

    if (goal.waypoints.empty())
    {
        ROS_WARN("[CommandExecutor] navigation_task 没有有效目标点，取消发送");
        complete_navigation_task_result(nav_task_attempt_id, false, "No valid target points");
        return;
    }
}

void CommandExecutor::handle_set_dock_pose(const json& data) {
    const json& payload = data.contains("data") && data["data"].is_object()
        ? data["data"]
        : data;
    const json& pose_json = payload.contains("pose") && payload["pose"].is_object()
        ? payload["pose"]
        : payload;
    const json& position_json = pose_json.contains("position") && pose_json["position"].is_object()
        ? pose_json["position"]
        : pose_json;

    auto read_number = [](const json& obj, const char* key, double& value) -> bool {
        if (!obj.contains(key) || !obj[key].is_number()) {
            return false;
        }
        value = obj[key].get<double>();
        return true;
    };

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!read_number(position_json, "x", x) || !read_number(position_json, "y", y)) {
        json ack = {
            {"type", "ack_set_dock_pose"},
            {"timestamp", static_cast<int>(time(nullptr))},
            {"localTime", getTimeNow()},
            {"device_id", device_id_},
            {"code", 1},
            {"status", "failed"},
            {"message", "dock pose requires numeric x and y"}
        };
        send_to_cloud(ack.dump());
        ROS_WARN("[DockPose] invalid payload: %s", data.dump().c_str());
        LOG_WARN("[DockPose] invalid payload={}", data.dump());
        return;
    }
    read_number(position_json, "z", z);

    json orientation_json = json::object();
    if (pose_json.contains("orientation") && pose_json["orientation"].is_object()) {
        orientation_json = pose_json["orientation"];
    } else if (payload.contains("orientation") && payload["orientation"].is_object()) {
        orientation_json = payload["orientation"];
    }

    double ox = 0.0;
    double oy = 0.0;
    double oz = 0.0;
    double ow = 1.0;
    read_number(orientation_json, "x", ox);
    read_number(orientation_json, "y", oy);
    read_number(orientation_json, "z", oz);
    read_number(orientation_json, "w", ow);

    geometry_msgs::PoseStamped dock_pose;
    dock_pose.header.stamp = ros::Time::now();
    dock_pose.header.frame_id = payload.value(
        "frame_id",
        payload.contains("header") && payload["header"].is_object()
            ? payload["header"].value("frame_id", "map")
            : std::string("map"));
    dock_pose.pose.position.x = x;
    dock_pose.pose.position.y = y;
    dock_pose.pose.position.z = z;
    dock_pose.pose.orientation.x = ox;
    dock_pose.pose.orientation.y = oy;
    dock_pose.pose.orientation.z = oz;
    dock_pose.pose.orientation.w = ow;

    dock_pose_pub_.publish(dock_pose);

    json ack = {
        {"type", "ack_set_dock_pose"},
        {"timestamp", static_cast<int>(time(nullptr))},
        {"localTime", getTimeNow()},
        {"device_id", device_id_},
        {"code", 0},
        {"status", "success"},
        {"message", "dock pose published"},
        {"frame_id", dock_pose.header.frame_id},
        {"position", {{"x", x}, {"y", y}, {"z", z}}},
        {"orientation", {{"x", ox}, {"y", oy}, {"z", oz}, {"w", ow}}}
    };
    send_to_cloud(ack.dump());
    ROS_INFO("[DockPose] published /dock_pose: frame_id=%s, x=%.3f, y=%.3f, z=%.3f",
             dock_pose.header.frame_id.c_str(), x, y, z);
    LOG_INFO("[DockPose] published payload={}", ack.dump());
}

void CommandExecutor::handle_stop_navigation(const json& data) {
    ROS_INFO("    ======================= 停止导航 (handle_stop_navigation) =======================    ");
    LOG_INFO(" 停止导航 (handle_stop_navigation) ... ");
    LOG_INFO("[StopNavigation] begin, payload={}", data.dump());
    log_runtime_flags("stop_navigation_begin");

    pid_t nav_pid = -1;
    pid_t goal_pid = -1;
    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        nav_pid = nav_process_;
        goal_pid = goal_process_;
    }

    if (nav_pid > 0) {
        LOG_INFO("[StopNavigation] send SIGINT to navigation.sh process group, pid={}", nav_pid);
        kill(-nav_pid, SIGINT);
        int status = 0;
        waitpid(nav_pid, &status, 0);
        log_wait_status("navigation.sh", status);
        std::lock_guard<std::mutex> lock(process_mutex_);
        if (nav_process_ == nav_pid) {
            nav_process_ = -1;
        }
    }

    if (goal_pid > 0) {
        LOG_INFO("[StopNavigation] send SIGINT to nav_to_goal.sh process group, pid={}", goal_pid);
        kill(-goal_pid, SIGINT);
        int status = 0;
        waitpid(goal_pid, &status, 0);
        log_wait_status("nav_to_goal.sh", status);
        std::lock_guard<std::mutex> lock(process_mutex_);
        if (goal_process_ == goal_pid) {
            goal_process_ = -1;
        }
    }

    std::string kill_scripts = scripts_path_ + "/kill_all.sh";
    std::string kill_command = script_log_command(kill_scripts, "kill_all");
    LOG_INFO("[StopNavigation] run cleanup script: {}", kill_scripts);
    log_file_access("kill_all.sh", kill_scripts, F_OK | R_OK);

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/bash", "bash", "-c", kill_command.c_str(), (char*)nullptr);

        // exec 失败
        std::fprintf(stderr, "[StopNavigation] execl failed for kill_all.sh, path=%s, errno=%d, error=%s\n",
                     kill_scripts.c_str(), errno, std::strerror(errno));
        perror("execl failed");
        exit(127);
    } 
    else if (pid > 0) {
        int status = -1;

        int wait_sec = 5;
        while (wait_sec-- > 0) {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid) break;
            if (ret < 0) {
                LOG_ERROR("[StopNavigation] waitpid failed for kill_all.sh, pid={}, errno={}, error={}",
                          pid, errno, std::strerror(errno));
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (wait_sec <= 0) {
            LOG_ERROR("kill_all.sh 超时，强制杀死");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
        }

        log_wait_status("kill_all.sh", status);
    } 
    else {
        LOG_ERROR("fork 失败");
        LOG_ERROR("[StopNavigation] fork failed for kill_all.sh, errno={}, error={}",
                  errno, std::strerror(errno));
    }

    std::string residual_cleanup =
        "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
        "rosnode kill /globalPlanner /pointcloud_tomography /localPlanner /navActionServer /taskRvizServer >/dev/null 2>&1 || true; "
        "pkill -9 -f '[p]ct_planner global_planner.launch' || true; "
        "pkill -9 -f '[g]lobalPlanner.py' || true; "
        "pkill -9 -f '[l]ocalPlanner.py' || true; "
        "pkill -9 -f '[n]avActionServer.py' || true; "
        "pkill -9 -f '[p]ointcloud_tomography' || true; "
        "pkill -9 -f '[b]ase_link.py' || true; "
        "pkill -9 -f '[f]lat_neupan_map.py' || true; "
        "timeout 3s bash -c 'yes y | rosnode cleanup' >/dev/null 2>&1 || true";
    std::string residual_cleanup_cmd = "bash -lc " + shell_quote(residual_cleanup);
    LOG_INFO("[StopNavigation] run residual cleanup command: {}", residual_cleanup_cmd);
    int residual_status = system(residual_cleanup_cmd.c_str());
    log_wait_status("navigation residual cleanup", residual_status);

    std::string current_map_id;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_map_id = current_map_id_;
    }

    json msg3 = {
        {"type", "ack_stop_navigation"},
        {"timestamp", (int)time(nullptr)},
        {"localTime", getTimeNow()},
        {"device_id", device_id_},
        {"code", 0},
        {"map_id", current_map_id}
    };
    send_to_cloud(msg3.dump());
    LOG_INFO("开始路径上报: {}", msg3.dump());

    ROS_INFO("Navigation stopped and cleaned.");
    navigation_env_ready_.store(false, std::memory_order_release);
    localization_ready_.store(false, std::memory_order_release);
    upload_enable_.store(false, std::memory_order_release);
    if (robot_state_handle_) {
        robot_state_handle_->clear_navigation_environment();
    }
    log_runtime_flags("stop_navigation_done");
}

void CommandExecutor::handle_start_mapping(const json& data) {
    if (!mapping_handle_) {
        LOG_ERROR("[Mapping] handler is not initialized");
        send_to_cloud(json{
            {"type", "start_mapping"},
            {"localTime", getTimeNow()},
            {"message", "建图处理器未初始化"}
        }.dump());
        return;
    }
    mapping_handle_->handle_start_mapping(data);
}

void CommandExecutor::handle_start_sh(const json& data)
{
    // 清除掉 g_nav_map 文件夹下的内容
    std::string clear_cmd = "rm -rf " + map_dir_ + "/*";
    system(clear_cmd.c_str());
    std::cout << "clear_cmd ... success " << clear_cmd << std::endl;

    auto data_obj = data["data"];

    std::string storage_path = data_obj.value("storage_path", "");
    std::string uuid = data_obj.value("uuid", "");
    std::cout << "handle_start_sh 的 uuid " << uuid << std::endl;

    std::vector<std::string> parts;
    std::stringstream ss(storage_path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            parts.push_back(item);
        }
    }

    if (parts.size() < 2) {
        LOG_ERROR("invalid storage_path: %s", storage_path.c_str());
        return;
    }

    std::string address = parts[0];      // 坚米
    std::string display_name = parts[1]; // 三楼

    std::cout << "address: " << address << std::endl;
    std::cout << "display_name: " << display_name << std::endl;

    fs::copy(
        "/home/jianmi/g_build_map/gloabl_map.pcd",
        fs::path("/home/jianmi/g_nav_map") / "gloabl_map.pcd",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        "/home/jianmi/g_build_map/gloabl_map.pickle",
        fs::path("/home/jianmi/g_nav_map") / "gloabl_map.pickle",
        fs::copy_options::overwrite_existing
    );

    std::vector<std::string> download_map_files = {"navmap.pcd"};
    map_manager_->download_and_deploy(address, display_name, download_map_files, map_dir_);

    std::string sh_2 = "/home/jianmi/catkin_pct/Scripts/map_process2.sh"; // lightweight_map.npy 和 navmap.pickle
    std::cout << "map_process2.sh 执行成功... " << std::endl;

    // 执行脚本
    int ret1 = system(sh_2.c_str());
    if (ret1 != 0) {
        LOG_ERROR("22.sh execute failed");
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::vector<std::string> final_file = {"lightweight_map.npy", "navmap.pickle"};
    map_manager_->upload_map_sh(device_id_, address, display_name, map_dir_, final_file, storage_path, uuid);
    std::cout << "上传 lightweight_map.npy 和 navmap.pickle 文件成功... " << std::endl;

    fs::path uuid_dir =
        fs::path("/home/jianmi/map_manager_local")
        / uuid;

    fs::create_directories(uuid_dir);
    // std::cout << "uuid_dir ---> " << uuid_dir << std::endl;

    // 复制文件到 map_manager_local 文件夹下
    fs::copy(
        "/home/jianmi/g_nav_map/lightweight_map.npy",
        uuid_dir / "lightweight_map.npy",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        "/home/jianmi/g_nav_map/navmap.pickle",
        uuid_dir / "navmap.pickle",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        "/home/jianmi/g_nav_map/gloabl_map.pcd",
        uuid_dir / "gloabl_map.pcd",
        fs::copy_options::overwrite_existing
    );

    fs::copy(
        "/home/jianmi/g_nav_map/navmap.pcd",
        uuid_dir / "navmap.pcd",
        fs::copy_options::overwrite_existing
    );

    send_to_cloud(json{
        {"type", "nav-upload-successful-end"},
        {"localTime", getTimeNow()},
        {"code", 0},
        {"message", "建图流程已结束, 可以进行导航操作... "}
    }.dump());

}

void CommandExecutor::exec_upload_cmd(const json& data) {
    std::string clear_cmd = "rm -rf " + map_dir_ + "/*";
    system(clear_cmd.c_str());
    std::cout << "clear_cmd ... success " << clear_cmd << std::endl;
    
}

void CommandExecutor::handle_stop_and_uploadmap(const json& data) {
    if (stop_upload_running_.exchange(true)) {
        ROS_WARN("stop_and_uploadmap is already running.");
        LOG_WARN("[Mapping] stop_and_uploadmap ignored: already running");
        send_to_cloud(json{
            {"type", "stop_and_uploadmap"},
            {"localTime", getTimeNow()},
            {"code", -1},
            {"message", "结束建图操作正在进行, 请勿重复操作... "}
        }.dump());   
        return;
    }

    launch_background_task(std::thread(&CommandExecutor::stop_and_uploadmap_async, this, data));
}

void CommandExecutor::stop_and_uploadmap_async(json data) {
    struct FlagGuard {
        std::atomic<bool>& flag;
        ~FlagGuard() { flag.store(false); }
    } guard{stop_upload_running_};

    mapping_handle_->stop_and_uploadmap_async(data);
}

std::string CommandExecutor::get_last_storage_path() const {
    return map_manager_ ? map_manager_->get_last_storage_path() : std::string{};
}

void CommandExecutor::set_upload_enable(bool enable) { 
    upload_enable_.store(enable, std::memory_order_release); 
}

bool CommandExecutor::get_localization_ready() const { 
    return localization_ready_.load(std::memory_order_acquire); 
}

std::string CommandExecutor::get_current_map_id() const { 
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_map_id_; 
}

void CommandExecutor::set_current_map_id(const std::string& id) { 
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_map_id_ = id; 
}

std::string CommandExecutor::get_state_name() const {
    if (localization_ready_.load(std::memory_order_acquire)) {
        return "LocalizationReady";
    }
    if (navigation_env_ready_.load(std::memory_order_acquire)) {
        return "WaitingInitialPose";
    }
    return "Idle";
}

void CommandExecutor::doneCb(const actionlib::SimpleClientGoalState &state,
                             const pct_planner::MultiWaypointNavigationResultConstPtr &result)
{
    if (state == actionlib::SimpleClientGoalState::SUCCEEDED)
    {
        ROS_INFO("Action Succeeded!");
    }
    else
    {
        ROS_WARN("Action Finished in State [%s]", state.toString().c_str());
    }

    ROS_INFO("Result Message: %s", result->message.c_str());
}

void CommandExecutor::feedbackCb(const pct_planner::MultiWaypointNavigationFeedbackConstPtr &feedback)
{
    ROS_INFO("Distance to current goal: %.2f", feedback->distance_to_current_goal);
}

void CommandExecutor::enqueue_task_state_request(
    const std::string& request_type, bool target_paused) {
    bool should_process = false;
    {
        std::lock_guard<std::mutex> lock(task_state_mutex_);
        task_state_requests_.push_back({request_type, target_paused});
        if (!task_state_worker_running_) {
            task_state_worker_running_ = true;
            should_process = true;
        }
    }

    LOG_INFO("[TaskState] enqueue request={}, target={}, should_process={}",
             request_type, target_paused ? "paused" : "running", should_process);
    if (should_process) {
        process_next_task_state_request();
    }
}

void CommandExecutor::process_next_task_state_request() {
    while (true) {
        TaskStateRequest request;
        bool requires_toggle = false;
        bool current_paused = false;
        {
            std::lock_guard<std::mutex> lock(task_state_mutex_);
            if (task_state_transition_active_) {
                return;
            }
            if (task_state_requests_.empty()) {
                task_state_worker_running_ = false;
                return;
            }

            request = task_state_requests_.front();
            task_state_requests_.pop_front();
            current_paused = task_paused_;
            requires_toggle = request.target_paused != task_paused_;
            if (requires_toggle) {
                active_task_state_request_ = request;
                task_state_transition_active_ = true;
            }
        }

        if (!requires_toggle) {
            send_to_cloud(json{
                {"type", request.request_type},
                {"status", "success"},
                {"code", 0},
                {"localTime", getTimeNow()},
                {"task_state", current_paused ? "paused" : "running"},
                {"message", "Task is already in the requested state"}
            }.dump());
            LOG_INFO("[TaskState] idempotent request={}, state={}",
                     request.request_type, current_paused ? "paused" : "running");
            continue;
        }

        LOG_INFO("[TaskState] toggle request={}, target={}", request.request_type,
                 request.target_paused ? "paused" : "running");
        exec_rosservice_cmd("pause_or_resume_task", "/toggle_task_state_service");
        return;
    }
}

void CommandExecutor::complete_task_state_transition(bool success, int ret) {
    TaskStateRequest completed_request;
    bool current_paused = false;
    {
        std::lock_guard<std::mutex> lock(task_state_mutex_);
        if (!task_state_transition_active_) {
            LOG_WARN("[TaskState] completion received without active transition, ret={}", ret);
            return;
        }

        completed_request = active_task_state_request_;
        if (success) {
            task_paused_ = completed_request.target_paused;
        }
        current_paused = task_paused_;
        task_state_transition_active_ = false;
        active_task_state_request_ = {"", false};
    }

    send_to_cloud(json{
        {"type", completed_request.request_type},
        {"status", success ? "success" : "failed"},
        {"localTime", getTimeNow()},
        {"code", success ? 0 : 1},
        {"task_state", current_paused ? "paused" : "running"},
        {"service", "/toggle_task_state_service"},
        {"ret", ret},
        {"message", success ? "Task state changed" : "Failed to change task state"}
    }.dump());
    LOG_INFO("[TaskState] complete request={}, success={}, state={}, ret={}",
             completed_request.request_type, success,
             current_paused ? "paused" : "running", ret);

    finish_command("pause_or_resume_task");
    process_next_task_state_request();
}

// 新增四个功能： 开启任务、 暂停/继续任务、 清除打点、 结束任务、 开始回充、 结束回充
void CommandExecutor::exec_rosservice_cmd(const std::string& type, const std::string& service_name) {
    std::string final_cmd;
    if (type == "pause_or_resume_task") {
        final_cmd = fmt::format("rosservice call {}", service_name);
    } else {
        final_cmd = fmt::format(
            R"(gnome-terminal -- bash -c "
                rosservice call {} ;
                echo 'Terminal will close in 10 seconds...';
                sleep 10
            ")", service_name
        );
    }

    ROS_INFO("Execute CMD:");
    ROS_INFO("%s", final_cmd.c_str());

    std::thread([this, final_cmd, type, service_name]() {

        int ret = std::system(final_cmd.c_str());

        bool success = WIFEXITED(ret) && (WEXITSTATUS(ret) == 0);

        if (type == "pause_or_resume_task") {
            ROS_INFO("[%s] return: %d", type.c_str(), ret);
            complete_task_state_transition(success, ret);
            return;
        }

        if (!success)
        {
            send_to_cloud(json{
                {"type", type},
                {"service", service_name},
                {"status", "执行失败"},
                {"localTime", getTimeNow()},
                {"ret", ret}
            }.dump());
        }
        else if (type == "start_task")
        {
            std::lock_guard<std::mutex> lock(task_state_mutex_);
            if (!task_state_worker_running_) {
                task_paused_ = false;
            }
        }

        ROS_INFO("[%s] return: %d", type.c_str(), ret);
        finish_command(type);

    }).detach();
}
