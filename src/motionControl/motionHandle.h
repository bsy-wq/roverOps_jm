#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include <ros/ros.h>
#include <std_msgs/String.h>

#include "../include/common/json.hpp"
#include "../include/common/log_manager.h"

using json = nlohmann::json;

class MotionHandle {
public:
    explicit MotionHandle(ros::NodeHandle& nh);
    ~MotionHandle();

    MotionHandle(const MotionHandle&) = delete;
    MotionHandle& operator=(const MotionHandle&) = delete;

    void handle_action_control(const json& data);

private:
    void publish_motion_cmd(const std::string& target_mode,
                            double lin_x, double lin_y, double ang_z);
    void robot_state_callback(const std_msgs::String::ConstPtr& msg);
    void watchdog_loop();

    ros::Publisher motion_cmd_pub_;
    ros::Subscriber robot_state_sub_;
    std::atomic<bool> running_{false};
    std::thread watchdog_thread_;
    std::mutex motion_mutex_;
    std::string motion_target_mode_;
    double motion_lin_x_ = 0.0;
    double motion_lin_y_ = 0.0;
    double motion_ang_z_ = 0.0;
    bool motion_publish_active_ = false;
    std::chrono::steady_clock::time_point last_motion_command_time_{};
    std::chrono::steady_clock::time_point last_idle_stand_time_{};
    std::chrono::steady_clock::time_point last_pose_command_time_{};
    std::string last_pose_command_;
    std::atomic<bool> robot_state_received_{false};
    std::atomic<bool> robot_is_lie_down_{false};

    static constexpr int kMotionStopTimeoutMs_ = 2000;
    static constexpr int kPoseCommandDedupMs_ = 3000;
};
