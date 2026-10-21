#include "handlers/MapWorkflowHandler.h"

#include "CommandExecutor.h"

namespace {

const char* const kMapWorkflowCommands[] = {
    "start_mapping",
    "stop_and_uploadmap",
    "nav-upload-successful",
    "upload_require_file",
    "bsy_test",
};

}  // namespace

void MapWorkflowHandler::register_routes(command::HandlerRegistry& routes) {
    for (const char* cmd_type : kMapWorkflowCommands) {
        routes[cmd_type] = [this, cmd_type](const nlohmann::json& data) {
            handle(cmd_type, data);
        };
    }
}

void MapWorkflowHandler::handle(const std::string& cmd_type, const nlohmann::json& data) {
    if (cmd_type == "start_mapping") {
        executor_.handle_start_mapping_command(data);
    } else if (cmd_type == "stop_and_uploadmap") {
        executor_.report_command_received(cmd_type);
        executor_.handle_stop_and_uploadmap(data);
    } else if (cmd_type == "nav-upload-successful") {
        executor_.handle_nav_upload_successful_command(data);
    } else if (cmd_type == "upload_require_file") {
        executor_.report_command_received(cmd_type);
        executor_.exec_upload_cmd(cmd_type);
    } else if (cmd_type == "bsy_test") {
        executor_.handle_bsy_test_command();
    }
}
