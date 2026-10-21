#include "handlers/DeviceHandler.h"

#include "CommandExecutor.h"

namespace {

const char* const kDeviceCommands[] = {
    "action_control",
    "control",
    "set_position_upload_frequency",
    "hikvision_continuous",
    "hikvision",
    "set_param",
    "upload_broatcast",
    "follow",
};

}  // namespace

void DeviceHandler::register_routes(command::HandlerRegistry& routes) {
    for (const char* cmd_type : kDeviceCommands) {
        routes[cmd_type] = [this, cmd_type](const nlohmann::json& data) {
            handle(cmd_type, data);
        };
    }
}

void DeviceHandler::handle(const std::string& cmd_type, const nlohmann::json& data) {
    if (cmd_type == "action_control" || cmd_type == "control") {
        executor_.handle_action_control_command(cmd_type, data);
    } else if (cmd_type == "set_position_upload_frequency") {
        executor_.handle_set_position_upload_frequency(data);
    } else if (cmd_type == "hikvision_continuous" || cmd_type == "hikvision") {
        executor_.handle_hikvision_command(cmd_type, data);
    } else if (cmd_type == "set_param") {
        executor_.handle_set_param_command(data);
    } else if (cmd_type == "upload_broatcast") {
        executor_.handle_broadcast_command(data);
    } else if (cmd_type == "follow") {
        executor_.handle_follow_backend_command(data);
    }
}
