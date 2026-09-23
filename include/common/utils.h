#pragma once

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <ros/ros.h>
#include "json.hpp"
#include <ctime>
#include <iomanip>

using json = nlohmann::json;

inline std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(c);
        }
    }

    return escaped.str();
}

inline std::string exec_cmd(const std::string& cmd)
{
    char buffer[256];
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    int status = pclose(pipe);
    if (status != 0) {
        ROS_WARN("exec_cmd returned non-zero status: %d, cmd=%s", status, cmd.c_str());
    }
    return result;
}

inline static std::string shell_single_quote(const std::string& value)
{
    std::string quoted = "'";
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

inline static std::string trim_copy(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

inline static std::string text_after_marker(const std::string& value, const std::string& marker)
{
    const auto pos = value.find(marker);
    if (pos == std::string::npos) {
        return "";
    }
    return trim_copy(value.substr(pos + marker.size()));
}

inline std::string get_uuid(const std::string& address, const std::string& display_name, 
                            const std::string& http_ip, const int& http_port)
{
    json request = {
        {"map_type", "high"},
        {"address", address},
        {"display_name", display_name}
    };

    std::string cmd =
        // "curl -sS -m 10 -X POST http://58.240.76.186:2800/get_uuid "
        "curl -sS -m 10 -X POST http://" + http_ip + ":" + std::to_string(http_port) + "/get_uuid "
        "-H 'Content-Type: application/json' "
        "-d " + shell_single_quote(request.dump()) + " 2>&1";

    std::string response = exec_cmd(cmd);
    if (response.empty()) {
        ROS_ERROR("get_uuid failed: empty response, address=%s, display_name=%s",
                  address.c_str(), display_name.c_str());
        return "";
    }

    try {
        auto j = json::parse(response);
        if (j.contains("error")) {
            ROS_ERROR("get_uuid failed: %s, response=%s",
                      j.value("error", "unknown").c_str(), response.c_str());
            return "";
        }
        std::string uuid = j.value("uuid", "");
        if (uuid.empty()) {
            ROS_ERROR("get_uuid failed: missing uuid, response=%s", response.c_str());
        }
        return uuid;
    } catch (const std::exception& e) {
        ROS_ERROR("get_uuid parse failed: %s, raw response=%s", e.what(), response.c_str());
        return "";
    }
}

inline static long long unix_timestamp_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

inline static bool parse_completed_task_round(const std::string& message, int& round)
{
    const std::string suffix = "轮任务完成";
    const size_t marker_pos = message.find("第");
    if (marker_pos == std::string::npos) {
        return false;
    }

    size_t digits_begin = marker_pos + std::string("第").size();
    while (digits_begin < message.size() &&
           std::isspace(static_cast<unsigned char>(message[digits_begin]))) {
        ++digits_begin;
    }

    size_t digits_end = digits_begin;
    while (digits_end < message.size() &&
           std::isdigit(static_cast<unsigned char>(message[digits_end]))) {
        ++digits_end;
    }
    if (digits_end == digits_begin) {
        return false;
    }

    size_t suffix_pos = digits_end;
    while (suffix_pos < message.size() &&
           std::isspace(static_cast<unsigned char>(message[suffix_pos]))) {
        ++suffix_pos;
    }
    if (message.compare(suffix_pos, suffix.size(), suffix) != 0) {
        return false;
    }

    const std::string digits = message.substr(digits_begin, digits_end - digits_begin);
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(digits.c_str(), &end, 10);
    if (errno != 0 || end == digits.c_str() || *end != '\0' || value <= 0 ||
        value > std::numeric_limits<int>::max()) {
        return false;
    }

    round = static_cast<int>(value);
    return true;
}

inline std::string getTimeNow() {
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);

    return buffer;
}