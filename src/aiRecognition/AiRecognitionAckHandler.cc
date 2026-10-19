#include "AiRecognitionAckHandler.h"

#include "common/json.hpp"
#include <openssl/evp.h>
#include <ros/ros.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>

using json = nlohmann::json;

namespace {

std::string safe_file_component(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char c : value) {
        result += (std::isalnum(c) || c == '-' || c == '_') ? static_cast<char>(c) : '_';
    }
    return result.empty() ? "unknown" : result;
}

bool decode_base64(const std::string& encoded, std::vector<unsigned char>& decoded) {
    std::string input = encoded;
    const auto comma = input.find(',');
    if (input.rfind("data:", 0) == 0 && comma != std::string::npos) input.erase(0, comma + 1);
    input.erase(std::remove_if(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c);
    }), input.end());
    if (input.empty() || input.size() % 4 != 0) return false;

    decoded.resize((input.size() / 4) * 3);
    const int size = EVP_DecodeBlock(decoded.data(),
                                     reinterpret_cast<const unsigned char*>(input.data()),
                                     static_cast<int>(input.size()));
    if (size < 0) return false;
    size_t padding = 0;
    if (!input.empty() && input.back() == '=') ++padding;
    if (input.size() > 1 && input[input.size() - 2] == '=') ++padding;
    decoded.resize(static_cast<size_t>(size) - padding);
    return true;
}

const char* image_extension(const std::vector<unsigned char>& data) {
    if (data.size() >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') return ".png";
    if (data.size() >= 3 && data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff) return ".jpg";
    if (data.size() >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F') return ".gif";
    if (data.size() >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P') return ".webp";
    return ".bin";
}

}  // namespace

AiRecognitionAckHandler::AiRecognitionAckHandler(CommandExecutor& executor, std::string device_id, std::string image_directory)
    : executor_(executor) , device_id_(std::move(device_id)), image_directory_(std::move(image_directory)) {}

void AiRecognitionAckHandler::handle(const std::string& payload, bool binary) const {
    if (binary) return;

    json ack;
    try {
        ack = json::parse(payload);
    } catch (const std::exception& e) {
        ROS_WARN("[AI ACK] invalid JSON: %s", e.what());
        return;
    }

    if (!ack.is_object() || ack.value("type", "") != "ack" ||
        ack.value("device", "") != "ai_recognition" ||
        ack.value("device_id", "") != device_id_ || ack.value("status", "") != "ok") return;

    const auto result = ack.find("result");
    if (result == ack.end() || !result->is_object()) return;
    const auto algorithms = result->find("algorithms");
    if (algorithms == result->end() || !algorithms->is_array()) return;

    try {
        std::filesystem::create_directories(image_directory_);
    } catch (const std::exception& e) {
        ROS_ERROR("[AI ACK] cannot create %s: %s", image_directory_.c_str(), e.what());
        return;
    }

    executor_.send_to_cloud(payload);
    LOG_INFO(payload);
    
    const std::string ack_id = safe_file_component(ack.value("id", "ack"));
    const long long seq = ack.value("seq", 0LL);
    for (size_t index = 0; index < algorithms->size(); ++index) {
        const auto& algorithm = (*algorithms)[index];
        if (!algorithm.is_object() || !algorithm.contains("image_base64") ||
            !algorithm["image_base64"].is_string()) continue;

        std::vector<unsigned char> image;
        if (!decode_base64(algorithm["image_base64"].get<std::string>(), image) || image.empty()) {
            ROS_WARN("[AI ACK] invalid image_base64 at algorithms[%zu]", index);
            continue;
        }

        const std::string algorithm_type = safe_file_component(algorithm.value("type", "algorithm"));
        const long long timestamp = algorithm.value("timestamp", ack.value("timestamp", 0LL));
        // const auto filename = ack_id + "_" + std::to_string(seq) + "_" + algorithm_type + "_" + std::to_string(timestamp) + "_" + std::to_string(index) + image_extension(image);
        const auto filename = algorithm_type + "_" + std::to_string(timestamp) + "_" + std::to_string(index) + image_extension(image); // visual_inspection_1791884368_0.jpg
        const auto path = std::filesystem::path(image_directory_) / filename;
        std::ofstream file(path, std::ios::binary);
        if (!file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()))) {
            ROS_ERROR("[AI ACK] failed to write %s", path.c_str());
            continue;
        }
        ROS_INFO("[AI ACK] saved %s", path.c_str());
    }
}
