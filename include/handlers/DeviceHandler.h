#pragma once

#include <string>

#include "handlers/HandlerRegistry.h"

class CommandExecutor;

// Routes external device commands (motion control, Hikvision gimbal, parameter
// updates, broadcast download, follow backend) to CommandExecutor.
//
// Phase 1 extraction: routing only, implementation stays in CommandExecutor.
class DeviceHandler {
public:
    explicit DeviceHandler(CommandExecutor& executor) : executor_(executor) {}

    void register_routes(command::HandlerRegistry& routes);

private:
    void handle(const std::string& cmd_type, const nlohmann::json& data);

    CommandExecutor& executor_;
};
