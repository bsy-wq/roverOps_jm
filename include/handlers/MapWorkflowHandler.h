#pragma once

#include <string>

#include "handlers/HandlerRegistry.h"

class CommandExecutor;

// Routes map workflow commands (start mapping, stop/upload map, upload
// completion, required-file upload, upload smoke test) to CommandExecutor.
//
// Phase 1 extraction: routing only, implementation stays in CommandExecutor.
class MapWorkflowHandler {
public:
    explicit MapWorkflowHandler(CommandExecutor& executor) : executor_(executor) {}

    void register_routes(command::HandlerRegistry& routes);

private:
    void handle(const std::string& cmd_type, const nlohmann::json& data);

    CommandExecutor& executor_;
};
