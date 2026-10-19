#pragma once

#include <string>
#include <functional>
#include "../include/common/json.hpp"

using json = nlohmann::json;

class Follow;

class Follow {
public:
    using SendToCloud = std::function<void(const std::string&)>;

    Follow(std::string device_id, 
           SendToCloud send_to_cloud,
           std::string base_url = "http://127.0.0.1:8088");

    void handle_follow_function(const json& data);

private:
    std::string device_id_;
    SendToCloud send_to_cloud_;
    std::string base_url_;

};
