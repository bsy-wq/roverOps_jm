#include "hikvihandle.h"

#include "../../include/common/log_manager.h"

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace {

constexpr int kContinuousPanSpeed = 10;
constexpr int kContinuousTiltSpeed = 10;
constexpr int kContinuousZoomSpeed = 0;
constexpr int kContinuousTimeoutMs = 800;
constexpr int kContinuousWatchdogPeriodMs = 100;

void init_curl_once() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
}

size_t write_callback(char* data, size_t size, size_t count, void* user_data) {
    const size_t bytes = size * count;
    auto* output = static_cast<std::string*>(user_data);
    output->append(data, bytes);
    return bytes;
}

xmlNodePtr find_element(xmlNodePtr node, const char* name) {
    for (xmlNodePtr current = node; current != nullptr; current = current->next) {
        if (current->type == XML_ELEMENT_NODE &&
            xmlStrEqual(current->name, BAD_CAST name)) {
            return current;
        }

        if (current->children != nullptr) {
            xmlNodePtr child = find_element(current->children, name);
            if (child != nullptr) {
                return child;
            }
        }
    }
    return nullptr;
}

bool parse_double_text(const char* text, double& value) {
    if (text == nullptr) {
        return false;
    }

    while (std::isspace(static_cast<unsigned char>(*text))) {
        ++text;
    }

    char* end = nullptr;
    const double parsed = std::strtod(text, &end);
    if (end == text || !std::isfinite(parsed)) {
        return false;
    }
    while (std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (*end != '\0') {
        return false;
    }

    value = parsed;
    return true;
}

bool read_node_double(xmlNodePtr node, double& value) {
    if (node == nullptr) {
        return false;
    }

    xmlChar* content = xmlNodeGetContent(node);
    if (content == nullptr) {
        return false;
    }

    const bool ok = parse_double_text(reinterpret_cast<const char*>(content), value);
    xmlFree(content);
    return ok;
}

bool read_property_double(xmlNodePtr node, const char* property, double& value) {
    if (node == nullptr) {
        return false;
    }

    xmlChar* content = xmlGetProp(node, BAD_CAST property);
    if (content == nullptr) {
        return false;
    }

    const bool ok = parse_double_text(reinterpret_cast<const char*>(content), value);
    xmlFree(content);
    return ok;
}

bool parse_position_xml(const std::string& body,
                        double& elevation,
                        double& azimuth,
                        double& zoom,
                        bool& has_zoom) {
    xmlDocPtr doc = xmlReadMemory(
        body.data(),
        static_cast<int>(body.size()),
        "hikvision_position.xml",
        nullptr,
        XML_PARSE_NOBLANKS
    );
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr elevation_node = find_element(root, "elevation");
    xmlNodePtr azimuth_node = find_element(root, "azimuth");
    xmlNodePtr zoom_node = find_element(root, "absoluteZoom");

    const bool position_ok =
        read_node_double(elevation_node, elevation) &&
        read_node_double(azimuth_node, azimuth);

    has_zoom = zoom_node != nullptr && read_node_double(zoom_node, zoom);
    if (!has_zoom) {
        zoom = 0.0;
    }

    xmlFreeDoc(doc);
    return position_ok;
}

bool parse_capabilities_xml(const std::string& body,
                            double& elevation_min,
                            double& elevation_max,
                            double& azimuth_min,
                            double& azimuth_max) {
    xmlDocPtr doc = xmlReadMemory(
        body.data(),
        static_cast<int>(body.size()),
        "hikvision_capabilities.xml",
        nullptr,
        XML_PARSE_NOBLANKS
    );
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr elevation_node = find_element(root, "elevation");
    xmlNodePtr azimuth_node = find_element(root, "azimuth");

    const bool capabilities_ok =
        read_property_double(elevation_node, "min", elevation_min) &&
        read_property_double(elevation_node, "max", elevation_max) &&
        read_property_double(azimuth_node, "min", azimuth_min) &&
        read_property_double(azimuth_node, "max", azimuth_max);

    xmlFreeDoc(doc);
    return capabilities_ok;
}

std::string format_angle(double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << value;
    return output.str();
}

double normalize_azimuth(double value) {
    value = std::fmod(value, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }
    if (value >= 360.0) {
        value -= 360.0;
    }
    return value;
}

bool in_range(double value, double minimum, double maximum) {
    constexpr double kTolerance = 1e-6;
    return value >= minimum - kTolerance && value <= maximum + kTolerance;
}

const json& get_command_payload(const json& data) {
    if (data.is_object() &&
        data.contains("data") &&
        data["data"].is_object()) {
        return data["data"];
    }
    return data;
}

std::string get_command(const json& payload) {
    if (!payload.is_object()) {
        return {};
    }

    if (payload.contains("commond") && payload["commond"].is_string()) {
        return payload["commond"].get<std::string>();
    }
    if (payload.contains("command") && payload["command"].is_string()) {
        return payload["command"].get<std::string>();
    }
    return {};
}

std::string get_mode(const json& payload) {
    if (!payload.is_object() ||
        !payload.contains("mode") ||
        !payload["mode"].is_string()) {
        return {};
    }
    return payload["mode"].get<std::string>();
}

std::string normalize_continuous_command(std::string command) {
    if (command == "turn_left" || command == "move_left") {
        return "left";
    }
    if (command == "turn_right" || command == "move_right") {
        return "right";
    }
    if (command == "turn_up" || command == "move_up") {
        return "up";
    }
    if (command == "turn_down" || command == "move_down") {
        return "down";
    }
    return command;
}

bool is_keep_continuous_command(const std::string& command) {
    return command == "keep_left" ||
           command == "keep_right" ||
           command == "keep_up" ||
           command == "keep_down" ||
           command == "keep_stop";
}

bool parse_angle(const json& payload, double& angle) {
    angle = 0.0;
    if (!payload.is_object()) {
        return true;
    }

    const json* raw_angle = nullptr;
    if (payload.contains("angle_deg")) {
        raw_angle = &payload["angle_deg"];
    } else if (payload.contains("angle")) {
        raw_angle = &payload["angle"];
    } else {
        return true;
    }

    try {
        if (raw_angle->is_number()) {
            angle = raw_angle->get<double>();
            return true;
        }
        if (raw_angle->is_string()) {
            size_t parsed_chars = 0;
            const std::string text = raw_angle->get<std::string>();
            angle = std::stod(text, &parsed_chars);
            return parsed_chars == text.size();
        }
    } catch (...) {
        return false;
    }
    return false;
}

}  // namespace

HikvisionHandle::HikvisionHandle(std::string ip,
                                 std::string username,
                                 std::string password,
                                 int channel)
    : ip_(std::move(ip)),
      username_(std::move(username)),
      password_(std::move(password)),
      channel_(channel) {
    const std::string base_url = "http://" + ip_;
      position_url_ = base_url + "/ISAPI/PTZCtrl/channels/" +
                    std::to_string(channel_) + "/absoluteEx";
    capabilities_url_ = position_url_ + "/capabilities";
    continuous_url_ = base_url + "/ISAPI/PTZCtrl/channels/" +
                      std::to_string(channel_) + "/continuous";
    continuous_watchdog_thread_ =
        std::thread(&HikvisionHandle::continuous_watchdog_loop, this);
}

HikvisionHandle::~HikvisionHandle() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        continuous_watchdog_running_ = false;
        continuous_active_ = false;
    }

    if (continuous_watchdog_thread_.joinable()) {
        continuous_watchdog_thread_.join();
    }

    // Stop the camera before the HTTP handle is destroyed.
    std::lock_guard<std::mutex> lock(mutex_);
    std::string error_message;
    set_continuous_velocity(0, 0, 0, error_message);
}

HikvisionHandle::HttpResponse HikvisionHandle::request(
    const std::string& method,
    const std::string& url,
    const std::string& body) const {
    init_curl_once();

    HttpResponse response;
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        response.error_message = "初始化海康HTTP客户端失败";
        return response;
    }

    const std::string user_password = username_ + ":" + password_;
    char error_buffer[CURL_ERROR_SIZE] = {0};
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/xml");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(curl, CURLOPT_USERPWD, user_password.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }

    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        response.error_message =
            error_buffer[0] != '\0' ? error_buffer : curl_easy_strerror(result);
    } else {
        response.transport_ok = true;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

bool HikvisionHandle::get_position(Position& position,
                                   std::string& error_message) const {
    const HttpResponse response = request("GET", position_url_);
    if (!response.transport_ok) {
        error_message = "读取海康云台位置失败: " + response.error_message;
        return false;
    }
    if (response.status_code < 200 || response.status_code >= 300) {
        error_message = "读取海康云台位置失败，HTTP状态码: " +
                        std::to_string(response.status_code);
        return false;
    }

    if (!parse_position_xml(
            response.body,
            position.elevation,
            position.azimuth,
            position.zoom,
            position.has_zoom)) {
        error_message = "无法解析PTZ位置";
        return false;
    }
    return true;
}

bool HikvisionHandle::get_capabilities(Capabilities& capabilities,
                                       std::string& error_message) const {
    const HttpResponse response = request("GET", capabilities_url_);
    if (!response.transport_ok) {
        error_message = "读取海康云台能力范围失败: " + response.error_message;
        return false;
    }
    if (response.status_code < 200 || response.status_code >= 300) {
        error_message = "读取海康云台能力范围失败，HTTP状态码: " +
                        std::to_string(response.status_code);
        return false;
    }

    if (!parse_capabilities_xml(
            response.body,
            capabilities.elevation_min,
            capabilities.elevation_max,
            capabilities.azimuth_min,
            capabilities.azimuth_max)) {
        error_message = "无法解析PTZ能力";
        return false;
    }
    return true;
}

bool HikvisionHandle::set_position(const Position& position,
                                   std::string& error_message) const {
    std::ostringstream body;
    body << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<PTZAbsoluteEx version=\"2.0\" "
            "xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\n"
         << "    <elevation>" << format_angle(position.elevation)
         << "</elevation>\n"
         << "    <azimuth>" << format_angle(position.azimuth)
         << "</azimuth>\n";
    if (position.has_zoom) {
        body << "    <absoluteZoom>" << format_angle(position.zoom)
             << "</absoluteZoom>\n";
    }
    body << "</PTZAbsoluteEx>";

    const HttpResponse response = request("PUT", position_url_, body.str());
    if (!response.transport_ok) {
        error_message = "下发海康云台位置失败: " + response.error_message;
        return false;
    }
    if (response.status_code < 200 || response.status_code >= 300) {
        error_message = "下发海康云台位置失败，HTTP状态码: " +
                        std::to_string(response.status_code);
        return false;
    }
    return true;
}

bool HikvisionHandle::set_continuous_velocity(int pan_speed,
                                              int tilt_speed,
                                              int zoom_speed,
                                              std::string& error_message) const {
    std::ostringstream body;
    body << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<PTZData version=\"2.0\" "
            "xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\n"
         << "    <pan>" << pan_speed << "</pan>\n"
         << "    <tilt>" << tilt_speed << "</tilt>\n"
         << "    <zoom>" << zoom_speed << "</zoom>\n"
         << "</PTZData>";

    const HttpResponse response = request("PUT", continuous_url_, body.str());
    if (!response.transport_ok) {
        error_message = "下发海康云台连续速度失败: " + response.error_message;
        return false;
    }
    if (response.status_code < 200 || response.status_code >= 300) {
        error_message = "下发海康云台连续速度失败，HTTP状态码: " +
                        std::to_string(response.status_code);
        return false;
    }
    return true;
}

HikvisionHandle::Result HikvisionHandle::continuous_controller(
    const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    Result result;

    int pan_speed = 0;
    int tilt_speed = 0;
    if (command == "left") {
        pan_speed = -kContinuousPanSpeed;
    } else if (command == "right") {
        pan_speed = kContinuousPanSpeed;
    } else if (command == "up") {
        tilt_speed = kContinuousTiltSpeed;
    } else if (command == "down") {
        tilt_speed = -kContinuousTiltSpeed;
    } else if (command == "stop" || command == "back") {
        // stop and back both mean stop for continuous control.
    } else {
        result.error_message = "不支持的海康连续云台指令: " + command;
        return result;
    }

    std::string error_message;
    if (!set_continuous_velocity(
            pan_speed, tilt_speed, kContinuousZoomSpeed, error_message)) {
        result.error_message = error_message;
        return result;
    }

    if (pan_speed == 0 && tilt_speed == 0) {
        continuous_active_ = false;
    } else {
        continuous_active_ = true;
        last_continuous_command_time_ = std::chrono::steady_clock::now();
    }

    result.success = true;
    result.pan_speed = pan_speed;
    result.tilt_speed = tilt_speed;
    result.timeout_ms = kContinuousTimeoutMs;
    return result;
}

json HikvisionHandle::execute_continuous_command(
    const std::string& response_type,
    const std::string& response_command,
    const std::string& command) {
    json result{{"type", response_type}};
    if (response_type == "hikvision_continuous") {
        result["command"] = response_command;
    } else {
        result["command"] = response_command;
        result["mode"] = "continuous";
    }

    const Result ptz_result = continuous_controller(command);
    if (ptz_result.success) {
        result["status"] = "success";
        result["pan_speed"] = ptz_result.pan_speed;
        result["tilt_speed"] = ptz_result.tilt_speed;
        result["timeout_ms"] = ptz_result.timeout_ms;

        Position position;
        std::string position_error;
        if (get_position(position, position_error)) {
            result["elevation"] = position.elevation;
            result["azimuth"] = position.azimuth;
        } else {
            result["position_error_message"] = position_error;
        }
        LOG_INFO("[Hikvision] continuous command={} succeeded, pan_speed={}, tilt_speed={}",
                 response_command, ptz_result.pan_speed, ptz_result.tilt_speed);
    } else {
        result["status"] = "failed";
        result["error_message"] = ptz_result.error_message;
        LOG_WARN("[Hikvision] continuous command={} failed, error={}",
                 response_command, ptz_result.error_message);
    }
    return result;
}

json HikvisionHandle::handle_continuous_command(const json& data) {
    LOG_INFO("[Hikvision] continuous command received, payload={}", data.dump());

    const json& ptz_payload = get_command_payload(data);
    const std::string command =
        normalize_continuous_command(get_command(ptz_payload));
    return execute_continuous_command(
        "hikvision_continuous", command, command);
}

json HikvisionHandle::handle_command(const json& data) {
    LOG_INFO("[Hikvision] command received, payload={}", data.dump());

    const json& ptz_payload = get_command_payload(data);
    const std::string mode = get_mode(ptz_payload);
    const std::string command = get_command(ptz_payload);
    if (mode == "continuous") {
        const std::string continuous_command =
            normalize_continuous_command(command);
        return execute_continuous_command(
            "hikvision", continuous_command, continuous_command);
    }
    if (!mode.empty() && mode != "relative") {
        return json{
            {"type", "hikvision"},
            {"mode", mode},
            {"command", command},
            {"status", "failed"},
            {"error_message", "不支持的海康云台控制模式: " + mode}
        };
    }
    if (is_keep_continuous_command(command)) {
        return execute_continuous_command(
            "hikvision", command, command.substr(5));
    }

    double angle = 0.0;
    json result{
        {"type", "hikvision"},
        {"mode", "relative"},
        {"command", command}
    };
    if (!parse_angle(ptz_payload, angle) || !std::isfinite(angle)) {
        result["status"] = "failed";
        result["error_message"] = "云台角度必须是数字";
        return result;
    }
    result["angle_deg"] = angle;

    const Result ptz_result = hikvision_controller(command, angle);
    if (ptz_result.success) {
        result["status"] = "success";
        result["elevation"] = ptz_result.elevation;
        result["azimuth"] = ptz_result.azimuth;
        LOG_INFO("[Hikvision] command={} succeeded, elevation={}, azimuth={}",
                 command, ptz_result.elevation, ptz_result.azimuth);
    } else {
        result["status"] = "failed";
        result["error_message"] = ptz_result.error_message;
        LOG_WARN("[Hikvision] command={} failed, error={}",
                 command, ptz_result.error_message);
    }
    return result;
}

void HikvisionHandle::continuous_watchdog_loop() {
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kContinuousWatchdogPeriodMs));

        std::lock_guard<std::mutex> lock(mutex_);
        if (!continuous_watchdog_running_) {
            return;
        }
        if (!continuous_active_) {
            continue;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_continuous_command_time_);
        if (elapsed.count() < kContinuousTimeoutMs) {
            continue;
        }

        std::string error_message;
        set_continuous_velocity(0, 0, 0, error_message);
        continuous_active_ = false;
    }
}

HikvisionHandle::Result HikvisionHandle::hikvision_controller(
    const std::string& command,
    double angle) {
    std::lock_guard<std::mutex> lock(mutex_);
    Result result;

    if (!std::isfinite(angle) || angle < 0.0) {
        result.error_message = "云台角度必须是大于等于0的数字";
        return result;
    }
    if (command != "up" && command != "down" &&
        command != "left" && command != "right" &&
        command != "back") {
        result.error_message = "不支持的海康云台指令: " + command;
        return result;
    }

    Position current;
    std::string error_message;
    if (!get_position(current, error_message)) {
        result.error_message = error_message;
        return result;
    }

    Capabilities capabilities;
    if (!get_capabilities(capabilities, error_message)) {
        result.error_message = error_message;
        return result;
    }

    Position target = current;
    if (command == "up") {
        target.elevation = current.elevation + angle;
    } else if (command == "down") {
        target.elevation = current.elevation - angle;
    } else if (command == "left") {
        target.azimuth = normalize_azimuth(current.azimuth - angle);
    } else if (command == "right") {
        target.azimuth = normalize_azimuth(current.azimuth + angle);
    } else if (command == "back") {
        target.elevation = 0.0;
        target.azimuth = 0.0;
    }

    if (!in_range(target.elevation,
                  capabilities.elevation_min,
                  capabilities.elevation_max) ||
        !in_range(target.azimuth,
                  capabilities.azimuth_min,
                  capabilities.azimuth_max)) {
        result.error_message = "当前角度不可达";
        return result;
    }

    if (!set_position(target, error_message)) {
        result.error_message = error_message;
        return result;
    }

    result.success = true;
    result.elevation = target.elevation;
    result.azimuth = target.azimuth;
    return result;
}
