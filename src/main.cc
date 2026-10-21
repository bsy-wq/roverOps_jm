#include "gateway/gateway.h"
#include "../include/CommandExecutor.h"
#include "../include/common/version.h"
#include "bms/BmsForwarder.h"
#include "config/yamlConfig.h"
#include "../include/common/cmdline.h"
#include "modifyValue/modifyHandle.h"
#include "localWebsocket/LocalWebsocketServer.h"
#include "localWebsocket/LightPolicy.h"
#include <robot_gateway/GatewayStatus.h>
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

    // 44.100 板子获取 机器狗的 sn 号
    std::string robot_id = get_serial_number().empty() ? config.robot_id : get_serial_number();

    // 当前运控版本
    std::string motion_control_version = config.motion_control_version;

    ros::init(argc, argv, "leggedog_gateway_node");
    signal(SIGCHLD, SIG_DFL);

    ros::NodeHandle nh;
    std::cout << "Robot Navigation Bridge Starting..." << std::endl;

    std::string scripts_path = get_home_dir() + "/catkin_pct/Scripts/";
    std::string map_dir = get_home_dir() + "/g_nav_map";

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
    // The gateway must outlive the executor's destructor and its workers.
    gateway gw;
    CommandExecutor executor(nh, robot_id, config.http_ip, config.http_port,
                             motion_control_version, scripts_path, map_dir, map_files,
                             bms_info, doc, config.parameter_xml_path, config.parameter_yaml_path);
    bool has_light_bms = false;
    bool light_bms_error = false;
    double light_soc = 0.0;
    BmsForwarder bms_forwarder(bms_websocket_url, robot_id);
    BatteryProvider battery_provider(nh, robot_id, config.bms_model, robot_model);
    battery_provider.setCallback([&](const BatterySnapshot& battery) {
        has_light_bms = battery.received;
        light_bms_error = battery.fault;
        light_soc = battery.soc_percent;
        executor.updateBattery(battery);
        bms_forwarder.updateBattery(battery);
    });

    // 底层 AI 识别到回调到 Web 服务器并本地保存
    AiRecognitionAckHandler ai_ack_handler(executor, robot_id, get_home_dir() + "/yq");
    // Also stop callbacks before their targets unwind on early returns/exceptions.
    struct GatewayStopGuard {
        gateway& instance;
        ~GatewayStopGuard() { instance.stop(); }
    } gateway_stop_guard{gw};
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

    bool light_estop = false;
    bool light_lie_down = false;
    auto light_gateway_sub = nh.subscribe<robot_gateway::GatewayStatus>(
        "/gateway_status", 10,
        [&](const robot_gateway::GatewayStatus::ConstPtr& state) {
            light_estop = state->estop_active;
            light_lie_down = state->current_logical_mode == "lie_down" ||
                state->current_mode == "lie_down" || state->current_mode == "LieDown" ||
                state->current_mode == "LIE_DOWN";
        });

    // BMS, gateway state and this timer run on the single ROS spin thread.
    ros::WallTimer light_timer = nh.createWallTimer(ros::WallDuration(1.0),
        [&](const ros::WallTimerEvent&) {
            const auto values = selectLightValues(has_light_bms, light_bms_error,
                light_soc, config.light_low_battery_percent,
                executor.get_behavior_state().motion_mode,
                light_estop || executor.is_http_follow_estopped(), light_lie_down,
                executor.is_http_follow_running());
            local_ws.broadcast(LocalWebsocketServer::Service::Equipment,
                json({{"type", "light_controller"}, {"value", values}, {"timestamp", unix_timestamp_ms()}}).dump());
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
    // std::cout << "  Map Directory: " << map_dir << std::endl;

    bool success = gw.setup(ws_url);
    std::cout << "机器狗网关状态: " << success << std::endl;
    std::cout << "  Gateway State: " << gw.get_state_name() << std::endl;
    std::cout << "  Executor State: " << executor.get_state_name() << std::endl;
    if (!bms_forwarder.start()) {
        std::cerr << "[BMS] forwarder failed to start" << std::endl;
    }
    if (!battery_provider.start()) {
        ROS_ERROR("[BatteryProvider] Failed to start battery acquisition");
        local_ws.stop();
        return 1;
    }
    ros::spin();
    battery_provider.stop();
    light_timer.stop();
    gw.stop();
    local_ws.stop();
    bms_forwarder.stop();
    return 0;
}

/*

sudo apt install openssh-client sshpass

*/
