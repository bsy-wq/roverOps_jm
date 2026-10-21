/*

The feature was designed on September 17, 2026; send bms、robot_state、navigation_env_active、map by frequency of 5 Hz.

*/

#include "robotState.h"

#include <chrono>
#include <utility>

namespace {

using json = nlohmann::json;

long long unix_timestamp_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

}  // namespace

robotState::robotState(
    ros::NodeHandle& nh,
    std::string device_id,
    SendToCloud send_to_cloud)
    : device_id_(std::move(device_id)),
      send_to_cloud_(std::move(send_to_cloud))
{
    heartbeat_timer_ = nh.createTimer(
        ros::Duration(0.2), // websocket 实时返回消息频率，5hz
        [this](const ros::TimerEvent&) {
            publish_snapshot("heartbeat");
        });
}

robotState::~robotState()
{
    heartbeat_timer_.stop();
}

void robotState::read_ros_state_info_callback(const std_msgs::String::ConstPtr& msg)
{
    if (!msg) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        latest_robot_state_ = msg->data;
        has_robot_state_ = true;
    }
}

void robotState::updateBattery(const BatterySnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    latest_bms_info_ = snapshot.status_info;
    has_bms_info_ = snapshot.received;
}

void robotState::update_navigation_environment(
    bool active,
    const std::string& address,
    const std::string& display_name)
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = active;
        if (!address.empty() && !display_name.empty()) {
            map_info_ = {
                {"address", address},
                {"display_name", display_name}
            };
        } else {
            map_info_ = json::object();
        }
    }

    publish_snapshot("navigation_env");
}

void robotState::set_navigation_env_active(bool active)
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = active;
    }

    publish_snapshot("navigation_env");
}

void robotState::clear_navigation_environment()
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = false;
        map_info_ = json::object();
    }

    publish_snapshot("navigation_env");
}

void robotState::publish_snapshot(const std::string& updated_source)
{
    if (!send_to_cloud_) {
        return;
    }

    json payload;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        payload = {
            {"type", "robot_status"},
            {"timestamp", unix_timestamp_ms()},
            {"device_id", device_id_},
            // {"updated_source", updated_source},
            {"has_robot_state", has_robot_state_},
            {"has_bms_info", has_bms_info_},
            {"navigation_env_active", navigation_env_active_},
            {"map", map_info_},
            {"version", "M20.01.02.20260923 B"}
        };

        if (has_robot_state_) {
            payload["robot_state"] = latest_robot_state_;
        }
        if (has_bms_info_) {
            payload["bms_info"] = latest_bms_info_;
        }
    }

    send_to_cloud_(payload.dump());
}
