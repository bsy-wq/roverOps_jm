#pragma once

#include "../../include/common/json.hpp"

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

using json = nlohmann::json;

class HikvisionHandle {
public:
    struct Result {
        bool success = false;
        double elevation = 0.0;
        double azimuth = 0.0;
        int pan_speed = 0;
        int tilt_speed = 0;
        int timeout_ms = 0;
        std::string error_message;
    };

    HikvisionHandle(std::string ip = "192.168.44.64",
                    std::string username = "admin",
                    std::string password = "yuanqi456",
                    int channel = 1);
    ~HikvisionHandle();

    Result hikvision_controller(const std::string& command, double angle);
    Result continuous_controller(const std::string& command);
    json handle_command(const json& data);
    json handle_continuous_command(const json& data);

private:
    struct HttpResponse {
        bool transport_ok = false;
        long status_code = 0;
        std::string body;
        std::string error_message;
    };

    struct Position {
        double elevation = 0.0;
        double azimuth = 0.0;
        double zoom = 0.0;
        bool has_zoom = false;
    };

    struct Capabilities {
        double elevation_min = 0.0;
        double elevation_max = 0.0;
        double azimuth_min = 0.0;
        double azimuth_max = 0.0;
    };

    HttpResponse request(const std::string& method,
                         const std::string& url,
                         const std::string& body = "") const;
    bool get_position(Position& position, std::string& error_message) const;
    bool get_capabilities(Capabilities& capabilities, std::string& error_message) const;
    bool set_position(const Position& position, std::string& error_message) const;
    bool set_continuous_velocity(int pan_speed,
                                 int tilt_speed,
                                 int zoom_speed,
                                 std::string& error_message) const;
    json execute_continuous_command(const std::string& response_type,
                                    const std::string& response_command,
                                    const std::string& command);
    void continuous_watchdog_loop();

    std::string ip_;
    std::string username_;
    std::string password_;
    int channel_;
    std::string position_url_;
    std::string capabilities_url_;
    std::string continuous_url_;
    mutable std::mutex mutex_;
    std::thread continuous_watchdog_thread_;
    bool continuous_watchdog_running_ = true;
    bool continuous_active_ = false;
    std::chrono::steady_clock::time_point last_continuous_command_time_;
};
