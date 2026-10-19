#pragma once

#include <yaml-cpp/yaml.h>
#include <string.h>
#include <stdexcept>

struct Config {

    std::string model;

    std::string robot_id;

    std::string http_ip;
    int http_port;

    std::string gateway_host;
    int gateway_port;
    std::string gateway_protocol;
    std::string endpoints_register;
    std::string endpoints_navbridge;

    int udp_port;

    std::string bms_model;

    std::string parameter_xml_path;
    std::string parameter_yaml_path;

    std::string websocket_ip;
    int websocket_port;
    std::string server_equipment;
    std::string server_test;
    double light_low_battery_percent = 20.0;
};

Config loadConfig(const std::string& path) {
    YAML::Node yaml = YAML::LoadFile(path);

    Config config;

    config.robot_id = yaml["robot"]["sn"].as<std::string>();
    config.model = yaml["robot"]["model"].as<std::string>();

    config.http_ip = yaml["network"]["http"]["ip"].as<std::string>();
    config.http_port = yaml["network"]["http"]["port"].as<int>();

    config.gateway_host = yaml["network"]["gateway"]["host"].as<std::string>();
    config.gateway_port = yaml["network"]["gateway"]["port"].as<int>();
    config.gateway_protocol = yaml["network"]["gateway"]["protocol"].as<std::string>();

    config.endpoints_register = yaml["network"]["endpoints"]["register"].as<std::string>();
    config.endpoints_navbridge = yaml["network"]["endpoints"]["navbridge"].as<std::string>();

    config.udp_port = yaml["udp"]["port"].as<int>();

    if (yaml["bms"] && yaml["bms"]["model"]) {
        config.bms_model = yaml["bms"]["model"].as<std::string>();
    } else if (yaml["bsm"] && yaml["bsm"]["model"]) {
        config.bms_model = yaml["bsm"]["model"].as<std::string>();
    } else {
        config.bms_model = "/m20/bms_state";
    }

    config.parameter_xml_path = yaml["parameter"]["xml_path"].as<std::string>();
    config.parameter_yaml_path = yaml["parameter"]["yaml_path"].as<std::string>();

    config.websocket_ip = yaml["websocket"]["ip"].as<std::string>();
    config.websocket_port = yaml["websocket"]["port"].as<int>();
    config.server_equipment = yaml["websocket"]["server_equipment"].as<std::string>();
    config.server_test = yaml["websocket"]["server_test"].as<std::string>();
    if (yaml["light"] && yaml["light"]["low_battery_percent"]) {
        config.light_low_battery_percent = yaml["light"]["low_battery_percent"].as<double>();
    }
    if (!(config.light_low_battery_percent >= 0.0 && config.light_low_battery_percent <= 100.0)) {
        throw std::runtime_error("light.low_battery_percent must be between 0 and 100");
    }

    return config;
}
