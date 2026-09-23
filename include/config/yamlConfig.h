#pragma once

#include <yaml-cpp/yaml.h>
#include <string.h>

struct Config {

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
};

Config loadConfig(const std::string& path) {
    YAML::Node yaml = YAML::LoadFile(path);

    Config config;
    config.robot_id = yaml["robot"]["sn"].as<std::string>();

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

    return config;
}