#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <ros/ros.h>
#include "common/json.hpp"

struct BatterySnapshot {
    bool received = false;
    bool valid = false;
    bool fault = false;
    double soc_percent = 0.0;
    std::chrono::steady_clock::time_point received_at{};
    nlohmann::json web_payload;
    nlohmann::json status_info = nlohmann::json::object();
};

class BatteryProvider {
public:
    using Callback = std::function<void(const BatterySnapshot&)>;
    BatteryProvider(ros::NodeHandle& nh, std::string robot_id,
                    std::string topic, std::string model);
    ~BatteryProvider();
    // Configure before start; invoked on the ROS subscriber thread.
    void setCallback(Callback callback);
    bool start();
    void stop();
    BatterySnapshot snapshot() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
