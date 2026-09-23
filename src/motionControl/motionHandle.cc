#include "motionHandle.h"

#include <cmath>

MotionHandle::MotionHandle(ros::NodeHandle& nh)
{
    motion_cmd_pub_ = nh.advertise<std_msgs::String>("/motion_cmd", 10);
    robot_state_sub_ = nh.subscribe(
        "/robot_state", 10, &MotionHandle::robot_state_callback, this);
    running_.store(true, std::memory_order_release);
    watchdog_thread_ = std::thread(&MotionHandle::watchdog_loop, this);
}

MotionHandle::~MotionHandle()
{
    running_.store(false, std::memory_order_release);
    if (watchdog_thread_.joinable()) {
        watchdog_thread_.join();
    }
}

void MotionHandle::publish_motion_cmd(
    const std::string& target_mode, double lin_x, double lin_y, double ang_z)
{
    json payload = {
        {"source", "navigation"},
        {"target_mode", target_mode},
        {"ttl_sec", 1.0},
        {"twist", {{"x", lin_x}, {"y", lin_y}, {"z", ang_z}}}
    };

    std_msgs::String msg;
    msg.data = payload.dump();
    motion_cmd_pub_.publish(msg);
}

void MotionHandle::robot_state_callback(const std_msgs::String::ConstPtr& msg)
{
    if (!msg) {
        return;
    }

    robot_is_lie_down_.store(msg->data == "lie_down", std::memory_order_release);
    robot_state_received_.store(true, std::memory_order_release);
}

void MotionHandle::watchdog_loop()
{
    constexpr auto kWatchdogInterval = std::chrono::milliseconds(100);
    constexpr auto kWebControlTimeout = std::chrono::milliseconds(kMotionStopTimeoutMs_);
    constexpr auto kStandPublishInterval = std::chrono::milliseconds(500);

    while (running_.load(std::memory_order_acquire) && ros::ok()) {
        bool web_control_timed_out = false;
        bool should_publish_idle_stand = false;
        {
            std::lock_guard<std::mutex> lock(motion_mutex_);
            const auto now = std::chrono::steady_clock::now();
            if (motion_publish_active_ &&
                now - last_motion_command_time_ > kWebControlTimeout) {
                motion_publish_active_ = false;
                web_control_timed_out = true;
            }

            const bool can_publish_idle_stand =
                robot_state_received_.load(std::memory_order_acquire) &&
                !robot_is_lie_down_.load(std::memory_order_acquire);
            if (!motion_publish_active_ && can_publish_idle_stand &&
                now - last_idle_stand_time_ >= kStandPublishInterval) {
                last_idle_stand_time_ = now;
                should_publish_idle_stand = true;
                publish_motion_cmd("stand", 0.0, 0.0, 0.0);
            }
        }

        if (web_control_timed_out) {
            ROS_INFO("[MotionWatchdog] Web control timeout, resume idle stand");
            LOG_INFO("[Motion] Web control timeout, resume idle stand");
        }
        if (should_publish_idle_stand) {
            ROS_DEBUG("[MotionWatchdog] published idle stand command");
        }
        std::this_thread::sleep_for(kWatchdogInterval);
    }
}

void MotionHandle::handle_action_control(const json& data)
{
    const json& payload =
        data.contains("data") && data["data"].is_object() ? data["data"] : data;
    const std::string web_type = data.value("type", "");
    const std::string web_device_id = data.value("device_id", data.value("robot_id", ""));
    const std::string web_timestamp =
        data.contains("timestamp") ? data["timestamp"].dump() : "";
    const std::string web_payload = payload.dump();
    ROS_INFO("[WebControl RX] type=%s device_id=%s timestamp=%s data=%s",
             web_type.c_str(), web_device_id.c_str(), web_timestamp.c_str(),
             web_payload.c_str());
    LOG_INFO("[WebControl RX] type={} device_id={} timestamp={} data={}",
             web_type, web_device_id, web_timestamp, web_payload);

    const std::string command = payload.value("command", data.value("command", ""));
    const json parameters =
        payload.contains("parameters") && payload["parameters"].is_object()
            ? payload["parameters"] : payload;

    auto read_velocity = [&parameters](const char* key, double& value) -> bool {
        value = 0.0;
        if (!parameters.contains(key)) {
            return true;
        }
        const json& raw = parameters[key];
        try {
            if (raw.is_number()) {
                value = raw.get<double>();
            } else if (raw.is_string()) {
                size_t parsed_chars = 0;
                const std::string text = raw.get<std::string>();
                value = std::stod(text, &parsed_chars);
                if (parsed_chars != text.size()) {
                    return false;
                }
            } else {
                return false;
            }
        } catch (...) {
            return false;
        }
        return std::isfinite(value);
    };

    const bool has_linear_velocity = parameters.contains("linear_velocity");
    const bool has_angular_velocity = parameters.contains("angular_velocity");
    const bool is_velocity_control =
        command.empty() && (has_linear_velocity || has_angular_velocity);
    const bool is_move_command =
        command == "move_forward" || command == "move_backward" ||
        command == "turn_left" || command == "turn_right" ||
        is_velocity_control;

    double linear_velocity = 0.0;
    double angular_velocity = 0.0;
    if (!read_velocity("linear_velocity", linear_velocity) ||
        !read_velocity("angular_velocity", angular_velocity)) {
        ROS_WARN("[Motion] control velocity must be a finite number: %s",
                 data.dump().c_str());
        return;
    }
    const bool is_zero_velocity =
        std::abs(linear_velocity) < 1e-6 && std::abs(angular_velocity) < 1e-6;

    bool should_publish_once = false;
    std::string publish_target_mode;
    double publish_lin_x = 0.0;
    double publish_lin_y = 0.0;
    double publish_ang_z = 0.0;

    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(motion_mutex_);
        if (command == "down" || command == "stand") {
            if (last_pose_command_ == command &&
                now - last_pose_command_time_ <
                    std::chrono::milliseconds(kPoseCommandDedupMs_)) {
                LOG_INFO("[Motion] duplicate pose command ignored within 3s: {}", command);
                return;
            }
            last_pose_command_ = command;
            last_pose_command_time_ = now;
            motion_target_mode_ = command == "down" ? "lie_down" : "stand";
            motion_lin_x_ = 0.0;
            motion_lin_y_ = 0.0;
            motion_ang_z_ = 0.0;
            motion_publish_active_ = false;
            should_publish_once = true;
        } else if (is_move_command) {
            const bool already_stopped =
                !motion_publish_active_ && std::abs(motion_lin_x_) < 1e-6 &&
                std::abs(motion_lin_y_) < 1e-6 && std::abs(motion_ang_z_) < 1e-6;
            if (is_zero_velocity && already_stopped) {
                LOG_DEBUG("[Motion] idle zero velocity frame ignored");
            } else {
                last_pose_command_.clear();
                motion_target_mode_ = "running";
                motion_lin_x_ = linear_velocity;
                motion_lin_y_ = 0.0;
                motion_ang_z_ = angular_velocity;
                last_motion_command_time_ = now;
                motion_publish_active_ = !is_zero_velocity;
                should_publish_once = true;
            }
        } else if (command == "stop") {
            last_pose_command_.clear();
            if (motion_target_mode_.empty()) {
                motion_target_mode_ = "running";
            }
            motion_lin_x_ = 0.0;
            motion_lin_y_ = 0.0;
            motion_ang_z_ = 0.0;
            motion_publish_active_ = false;
            should_publish_once = true;
        } else if (command == "emergency_stop") {
            last_pose_command_.clear();
            if (motion_target_mode_.empty()) {
                motion_target_mode_ = "running";
            }
            motion_lin_x_ = 0.0;
            motion_lin_y_ = 0.0;
            motion_ang_z_ = 0.0;
            motion_publish_active_ = false;
            should_publish_once = true;
            ROS_WARN("[Motion] emergency_stop received; velocity command was zeroed, "
                     "but /estop is unavailable because robot_gateway is not installed");
        } else if (command == "emergency_stop_release" ||
                   command == "emergency_stop_force_clear") {
            ROS_WARN("[Motion] %s received, but robot_gateway/Estop is unavailable",
                     command.c_str());
        } else {
            ROS_WARN("[Motion] unknown control command: %s", command.c_str());
            return;
        }

        publish_target_mode = motion_target_mode_;
        publish_lin_x = motion_lin_x_;
        publish_lin_y = motion_lin_y_;
        publish_ang_z = motion_ang_z_;
        last_motion_command_time_ = std::chrono::steady_clock::now();
        motion_publish_active_ = true;
        if (is_move_command) {
            LOG_DEBUG("[Motion] velocity command target_mode={}, lin_x={}, lin_y={}, ang_z={}",
                      motion_target_mode_, motion_lin_x_, motion_lin_y_, motion_ang_z_);
        } else {
            LOG_INFO("[Motion] control command={}, target_mode={}, lin_x={}, lin_y={}, ang_z={}",
                     command, motion_target_mode_, motion_lin_x_, motion_lin_y_, motion_ang_z_);
        }
    }

    if (should_publish_once) {
        publish_motion_cmd(publish_target_mode, publish_lin_x, publish_lin_y, publish_ang_z);
    }
}
