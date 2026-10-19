#pragma once

#include <string>
#include <tinyxml2.h>
#include <functional>
#include <variant>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "../include/common/json.hpp"

using json = nlohmann::json;

class ModifyHandle {
public:
    using SendToCloud = std::function<void(const std::string&)>;
    
    explicit ModifyHandle(
        tinyxml2::XMLDocument& doc, 
        SendToCloud send_to_cloud,
        const std::string& device_id,
        const std::string& parameter_path, const std::string& parameter_yaml_path);

    ~ModifyHandle();

    std::string getParam(const std::string& param_type, const std::string& name);
    bool setParam(const std::string& param, const std::string& name, const std::string& value);
    bool save(const std::string& file_path);
    bool updateParam(const json& res_json);

    bool setYamlValue(YAML::Node root, const std::string& path, const json& value);
    bool modifyYaml(const std::string& path, const json& value);

private:
    std::string device_id_;
    tinyxml2::XMLDocument& doc_;
    SendToCloud send_to_cloud_;
    std::string parameter_path_;
    std::string parameter_yaml_path_;
    
};
