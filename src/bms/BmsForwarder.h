#pragma once

#include <memory>
#include <string>
#include <functional>

#include <ros/ros.h>

class BmsForwarder {
public:
    BmsForwarder(ros::NodeHandle& nh, std::string websocket_url,
                 std::string robot_id, std::string bms_topic,
                 std::string robot_model = "M20");
    ~BmsForwarder();

    BmsForwarder(const BmsForwarder&) = delete;
    BmsForwarder& operator=(const BmsForwarder&) = delete;

    bool start();
    void stop();
    // Configure before start(). Called on the ROS subscriber thread.
    void setBatteryCallback(std::function<void(bool, double)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
