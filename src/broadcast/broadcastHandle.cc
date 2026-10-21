#include <iostream>
#include "broadcastHandle.h"
#include "../include/common/utils.h"

Broadcast::Broadcast(std::string device_id, std::string http_ip, int http_port,
                    SendToCloud send_to_cloud)

    : device_id_(std::move(device_id)),
      http_ip_(std::move(http_ip)),
      http_port_(std::move(http_port)),
      send_to_cloud_(std::move(send_to_cloud)) {}


bool Broadcast::download_broadcast(const std::string& filename, const std::string& http_ip, const int& http_port) {
    std::string cmd = "curl -f -L http://" + http_ip + ":" + std::to_string(http_port) +
                      "/download_broadcast/" + filename + " -o " + get_home_dir() + "/yq/" + filename;

    std::cout << "cmd: " << cmd << std::endl;

    std::string response = exec_cmd(cmd);

    if (!response.empty()) {
        send_to_cloud_(json{
            {"type", "download_broadcast"},
            {"code", -1},
            {"status", false},
            {"localTime", getTimeNow()},
            {"message", "下载喊话器文件失败"}
        }.dump());
        return false;
    }

    send_to_cloud_(json{
        {"type", "download_broadcast"},
        {"code", 0},
        {"status", true},
        {"localTime", getTimeNow()},
        {"message", "下载喊话器文件成功"}
    }.dump());

    // 临时文件上传到外部上面

    return true;
}