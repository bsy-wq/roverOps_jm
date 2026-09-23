#include "gateway/gateway.h"
#include "../include/CommandExecutor.h"
#include "../include/common/version.h"
#include "bms/BmsForwarder.h"
#include "config/yamlConfig.h"
#include "../include/common/cmdline.h"
#include "modifyValue/modifyHandle.h"
#include <array>
#include <fstream>
#include <string>
#include <iostream>
#include <tinyxml2.h>

#define ROBOT_DOG_VERSION "0.1.0"

using namespace tinyxml2;

int main(int argc, char* argv[]) {

    setlocale(LC_CTYPE, "zh_CN.utf8");
    
    cmdline::parser a;
    a.add("version", 'v', "show version");
    a.parse_check(argc, argv);
    if (a.exist("version")) {std::cout << "robot_dog_version: " << ROBOT_DOG_VERSION << std::endl;  return 0; }

    Config config = loadConfig("robot.yaml");

    XMLDocument doc;
    XMLError ret = doc.LoadFile(config.parameter_xml_path.c_str());
    if (ret != XML_SUCCESS) { std::cout << "加载 xml 失败: " << doc.ErrorStr() << std::endl; return -1; }

    ros::init(argc, argv, "leggedog_gateway_node");
    signal(SIGCHLD, SIG_DFL);

    ros::NodeHandle nh;
    std::cout << "Robot Navigation Bridge Starting..." << std::endl;

    std::string ws_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_navbridge + "/" + config.robot_id;

    std::string scripts_path = "/home/jianmi/catkin_pct/Scripts/";
    std::string map_dir = "/home/jianmi/g_nav_map";

    std::vector<std::string> map_files = {"gloabl_map.pcd", "gloabl_map.pickle"};

    nh.param("device_id", config.robot_id, config.robot_id);
    nh.param("http_ip", config.http_ip, config.http_ip);
    nh.param("scripts_path", scripts_path, scripts_path);
    nh.param("map_dir", map_dir, map_dir);

    std::string bms_websocket_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_register + "/" + config.robot_id;
    nh.param("bms_websocket_url", bms_websocket_url, bms_websocket_url);

    LOG_INFO("[Main] startup config: device_id={}, http_ip={}, scripts_path={}, map_dir={}",
             config.robot_id, config.http_ip, scripts_path, map_dir);
    
    gateway gw;
    gw.set_device_id(config.robot_id);
    const char *bms_info = config.bms_model.c_str();
    CommandExecutor executor(nh, config.robot_id, config.http_ip, config.http_port, scripts_path, map_dir, map_files, bms_info, doc, config.parameter_xml_path, config.parameter_yaml_path);
    BmsForwarder bms_forwarder(
        nh,
        bms_websocket_url,
        config.robot_id,
        config.bms_model);
    // ModifyHandle modifyhandle(doc);

    // 上传 sn 号到服务器上面
    executor.add_upload_sn(config.robot_id, config.http_ip, config.http_port);

    executor.setGateway(&gw);
    gw.setMessageCallback([&executor](const json& payload_json) {
        executor.execute(payload_json);
    });
    
    std::cout << "Robot Navigation Bridge Initialized." << std::endl;
    std::cout << "  WebSocket URL: " << ws_url << std::endl;
    std::cout << "  Device ID: " << config.robot_id << std::endl;
    std::cout << "  Map Directory: " << map_dir << std::endl;

    bool success = gw.setup(ws_url);
    std::cout << "机器狗网关状态: " << success << std::endl;
    std::cout << "  Gateway State: " << gw.get_state_name() << std::endl;
    std::cout << "  Executor State: " << executor.get_state_name() << std::endl;
    if (!bms_forwarder.start()) {
        std::cerr << "[BMS] forwarder failed to start" << std::endl;
    }
    ros::spin();
    bms_forwarder.stop();
    return 0;
}

/*
{"device_id":"M20-P1-bsy","has_bms_info":false,"has_robot_state":false,"map":{},"navigation_env_active":false,"timestamp":1788938004093,"type":"robot_state_info"}


{
    "ws_url": "ws://aifly.test.qxwz.com/novaapi/robot/device/jianmi/m20",
    "sn": "robot_001",
    "http_ip": "122.192.33.182"
}


*/
