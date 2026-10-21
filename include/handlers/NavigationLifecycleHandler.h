#pragma once

#include <string>

#include "handlers/HandlerRegistry.h"

class CommandExecutor;

// Routes navigation lifecycle commands (relocalization, navigation start,
// task control, dock pose, recharge) to CommandExecutor.
//
// Phase 1 extraction: only the routing table lives here. The heavy lifting is
// still in CommandExecutor so behavior, locking and thread ordering are
// unchanged. Moving the implementation in is a later, isolated step.
class NavigationLifecycleHandler {
public:
    explicit NavigationLifecycleHandler(CommandExecutor& executor) : executor_(executor) {}

    void register_routes(command::HandlerRegistry& routes);

private:
    void handle(const std::string& cmd_type, const nlohmann::json& data);

    CommandExecutor& executor_;
};
