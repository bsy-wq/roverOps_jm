#pragma once

#include <memory>
#include <string>

#include <ros/ros.h>

class BmsForwarder {
public:
    BmsForwarder(ros::NodeHandle& nh, std::string websocket_url,
                 std::string robot_id, std::string bms_topic);
    ~BmsForwarder();

    BmsForwarder(const BmsForwarder&) = delete;
    BmsForwarder& operator=(const BmsForwarder&) = delete;

    bool start();
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
