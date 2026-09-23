#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <ros/ros.h>

#include "common/json.hpp"
#include <robot_gateway/M20BmsState.h>
#include <std_msgs/String.h>

class robotState {

public:

    using SendToCloud = std::function<void(const std::string&)>;

    robotState(
        ros::NodeHandle& nh,
        std::string device_id,
        SendToCloud send_to_cloud
    );
    ~robotState();

    void read_ros_state_info_callback(const std_msgs::String::ConstPtr& msg);
    void read_ros_bms_info_callback(const robot_gateway::M20BmsState::ConstPtr& msg);
    void update_navigation_environment(
        bool active,
        const std::string& address,
        const std::string& display_name);
    void set_navigation_env_active(bool active);
    void clear_navigation_environment();

private:
    void publish_snapshot(const std::string& updated_source);

    std::string device_id_;
    SendToCloud send_to_cloud_;
    ros::Timer heartbeat_timer_;
    std::mutex state_mutex_;
    nlohmann::json latest_robot_state_;
    nlohmann::json latest_bms_info_;
    nlohmann::json map_info_ = nlohmann::json::object();
    bool has_robot_state_ = false;
    bool has_bms_info_ = false;
    bool navigation_env_active_ = false;

};
