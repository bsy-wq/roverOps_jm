#include "handlers/NavigationLifecycleHandler.h"

#include "CommandExecutor.h"

namespace {

const char* const kNavigationLifecycleCommands[] = {
    "initial_pose",
    "start_navigation",
    "navigation_task",
    "set_dock_pose",
    "start_task",
    "pause_task",
    "resume_task",
    "clear_waypoints",
    "cancel_task",
    "start_recharge",
    "stop_recharge",
};

}  // namespace

void NavigationLifecycleHandler::register_routes(command::HandlerRegistry& routes) {
    for (const char* cmd_type : kNavigationLifecycleCommands) {
        routes[cmd_type] = [this, cmd_type](const nlohmann::json& data) {
            handle(cmd_type, data);
        };
    }
}

void NavigationLifecycleHandler::handle(const std::string& cmd_type, const nlohmann::json& data) {
    if (cmd_type == "initial_pose") {
        executor_.report_command_received(cmd_type);
        executor_.handle_initial_pose(data);
    } else if (cmd_type == "start_navigation") {
        executor_.report_command_received(cmd_type);
        executor_.handle_start_navigation(data);
    } else if (cmd_type == "navigation_task") {
        executor_.report_command_received(cmd_type);
        executor_.handle_navigation_task(data);
    } else if (cmd_type == "set_dock_pose") {
        executor_.report_command_received(cmd_type);
        executor_.handle_set_dock_pose(data);
    } else if (cmd_type == "start_task") {
        executor_.report_command_received(cmd_type);
        executor_.exec_rosservice_cmd("start_task", "/start_task");
    } else if (cmd_type == "pause_task" || cmd_type == "resume_task") {
        executor_.handle_task_state_request(cmd_type);
    } else if (cmd_type == "clear_waypoints") {
        executor_.handle_clear_waypoints_request();
    } else if (cmd_type == "cancel_task") {
        executor_.report_command_received(cmd_type);
        executor_.exec_rosservice_cmd("cancel_task", "/cancel_task");
    } else if (cmd_type == "start_recharge") {  // 开始回充
        executor_.report_command_received(cmd_type);
        executor_.exec_rosservice_cmd("start_recharge", "/start_recharge");
    } else if (cmd_type == "stop_recharge") {  // 结束回充
        executor_.report_command_received(cmd_type);
        executor_.exec_rosservice_cmd("stop_recharge", "/stop_recharge");
    }
}
