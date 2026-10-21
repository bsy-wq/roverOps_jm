#pragma once

#include <string>
#include <functional>
#include <atomic>
#include "../include/common/json.hpp"

using json = nlohmann::json;

class Follow;

class Follow {
public:
    using SendToCloud = std::function<void(const std::string&)>;

    Follow(std::string device_id, 
           SendToCloud send_to_cloud,
           std::string base_url = "http://122.192.33.182:2802");

    bool handle_follow_function(const json& data);
    bool is_running() const { return running_.load(); }
    bool is_estopped() const { return estopped_.load(); }

private:
    std::string device_id_;
    SendToCloud send_to_cloud_;
    std::string base_url_;
    std::atomic<bool> running_{false};
    std::atomic<bool> estopped_{false};

};
