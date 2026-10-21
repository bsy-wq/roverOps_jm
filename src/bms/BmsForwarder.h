#pragma once

#include <memory>
#include <string>
#include <functional>

#include "BatteryProvider.h"

class BmsForwarder {
public:
    BmsForwarder(std::string websocket_url, std::string robot_id);
    ~BmsForwarder();

    BmsForwarder(const BmsForwarder&) = delete;
    BmsForwarder& operator=(const BmsForwarder&) = delete;

    bool start();
    void stop();
    void updateBattery(const BatterySnapshot& snapshot);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
