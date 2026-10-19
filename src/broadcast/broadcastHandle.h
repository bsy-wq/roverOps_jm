#pragma once

#include <string>
#include "../include/common/json.hpp"

using json = nlohmann::json;

class Broadcast;

class Broadcast {
public:
    using SendToCloud = std::function<void(const std::string&)>;

    Broadcast(std::string device_id, std::string http_ip, int http_port,
              SendToCloud send_to_cloud);

    bool download_broadcast(const std::string& filename, const std::string& http_ip, const int& http_port);

private:
    std::string device_id_;
    std::string http_ip_;
    int http_port_;
    SendToCloud send_to_cloud_;

};