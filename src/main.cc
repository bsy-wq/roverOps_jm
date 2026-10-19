#include "gateway/gateway.h"
#include "../include/CommandExecutor.h"
#include "../include/common/version.h"
#include "bms/BmsForwarder.h"
#include "config/yamlConfig.h"
#include "../include/common/cmdline.h"
#include "modifyValue/modifyHandle.h"
#include "localWebsocket/LocalWebsocketServer.h"
#include "localWebsocket/LightPolicy.h"
#include "aiRecognition/AiRecognitionAckHandler.h"
#include <array>
#include <fstream>
#include <string>
#include <iostream>
#include <tinyxml2.h>

using namespace tinyxml2;

int main(int argc, char* argv[]) {

    setlocale(LC_CTYPE, "zh_CN.utf8");

    Config config = loadConfig("robot.yaml");

    XMLDocument doc;
    XMLError ret = doc.LoadFile(config.parameter_xml_path.c_str());
    if (ret != XML_SUCCESS) { std::cout << "加载 xml 失败: " << doc.ErrorStr() << std::endl; }

    // 44.100板子获取 机器狗的 sn 号
    std::string robot_id = get_serial_number();

    ros::init(argc, argv, "leggedog_gateway_node");
    signal(SIGCHLD, SIG_DFL);

    ros::NodeHandle nh;
    std::cout << "Robot Navigation Bridge Starting..." << std::endl;

    std::string scripts_path = "/home/jianmi/catkin_pct/Scripts/";
    std::string map_dir = "/home/jianmi/g_nav_map";

    std::vector<std::string> map_files = {"gloabl_map.pcd", "gloabl_map.pickle"};

    nh.param("device_id", robot_id, robot_id);
    robot_id = trim_copy(robot_id);
    if (robot_id.empty()) {
        ROS_ERROR("[Main] Empty robot device_id; check serial number or ROS device_id parameter");
        return 1;
    }
    nh.param("http_ip", config.http_ip, config.http_ip);
    nh.param("scripts_path", scripts_path, scripts_path);
    nh.param("map_dir", map_dir, map_dir);

    std::string ws_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_navbridge + "/" + robot_id;
    std::string bms_websocket_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_register + "/" + robot_id;
    nh.param("bms_websocket_url", bms_websocket_url, bms_websocket_url);

    LOG_INFO("[Main] startup config: device_id={}, http_ip={}, scripts_path={}, map_dir={}",
             robot_id, config.http_ip, scripts_path, map_dir);

    LocalWebsocketServer local_ws(config.websocket_ip, config.websocket_port,
                                 config.server_equipment + "/" + robot_id, config.server_test);

    const char *bms_info = config.bms_model.c_str();
    std::string robot_model = config.model;
    CommandExecutor executor(nh, robot_id, config.http_ip, config.http_port, scripts_path, map_dir, map_files, bms_info, doc, config.parameter_xml_path, config.parameter_yaml_path);
    bool has_light_bms = false;
    bool light_bms_error = false;
    double light_soc = 0.0;
    BmsForwarder bms_forwarder(
        nh,
        bms_websocket_url,
        robot_id,
        config.bms_model,
        robot_model);
    bms_forwarder.setBatteryCallback([&](bool error, double soc) {
        has_light_bms = true;
        light_bms_error = error;
        light_soc = soc;
    });

    // 底层 AI 识别到回调到 Web 服务器并本地保存
    AiRecognitionAckHandler ai_ack_handler(executor, robot_id, "/home/jianmi/yq");
    gateway gw;
    if (!local_ws.start()) {
        ROS_ERROR("[Main] Failed to start local WebSocket services");
        return 1;
    }
    gw.set_device_id(robot_id);
    local_ws.setMessageHandler(LocalWebsocketServer::Service::Equipment,
        [&ai_ack_handler](LocalWebsocketServer::Handle,
                          const std::string& payload, bool binary) {
            ai_ack_handler.handle(payload, binary);
        });

    // 订阅 bms 电池数据和导航状态的回调函数在单个 ros::spin 线程上
    ros::WallTimer light_timer = nh.createWallTimer(ros::WallDuration(1.0),
        [&](const ros::WallTimerEvent&) {
            const auto status = selectLightStatus(has_light_bms, light_bms_error,
                light_soc, config.light_low_battery_percent,
                executor.get_behavior_state().motion_mode);                     
            local_ws.broadcast(LocalWebsocketServer::Service::Equipment,
                json({{"type", "light_controller"}, {"value", lightValue(status)}, "timestamp", unix_timestamp_ms()}).dump());
        });

    // 上传 sn 号到服务器上面
    executor.add_upload_sn(robot_id, config.http_ip, config.http_port);

    executor.setGateway(&gw);
    gw.setBroadcastCallback([&local_ws](const std::string& payload) {
        local_ws.broadcast(LocalWebsocketServer::Service::Equipment, payload);
    });
    gw.setMessageCallback([&executor](const json& payload_json) {
        executor.execute(payload_json);
    });
    
    std::cout << "Robot Navigation Bridge Initialized." << std::endl;
    std::cout << "  WebSocket URL: " << ws_url << std::endl;
    std::cout << "  Device ID: " << robot_id << std::endl;
    std::cout << "  Map Directory: " << map_dir << std::endl;

    bool success = gw.setup(ws_url);
    std::cout << "机器狗网关状态: " << success << std::endl;
    std::cout << "  Gateway State: " << gw.get_state_name() << std::endl;
    std::cout << "  Executor State: " << executor.get_state_name() << std::endl;
    if (!bms_forwarder.start()) {
        std::cerr << "[BMS] forwarder failed to start" << std::endl;
    }
    ros::spin();
    light_timer.stop();
    local_ws.stop();
    bms_forwarder.stop();
    return 0;
}

/*
sudo apt install openssh-client sshpass

1. include/common/utils.h:211：读取序列号后清理首尾空白。

   return trim_copy(result);  // 原来是 return result;
2. src/main.cc:43：读取 ROS 参数后，再清理最终机器狗 ID。

   robot_id = trim_copy(robot_id);

   同时增加空 ID 检查，并将 ws_url 拼接移到了 第 52 行，确保使用最终 ID。

3. src/gateway/gateway.cc:54：修复退出时的异常。

   if (ws_server.is_listening()) {
    websocketpp::lib::error_code ec;
    ws_server.stop_listening(ec);

*/
