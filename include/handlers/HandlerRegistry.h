#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "common/json.hpp"

namespace command {

// A handler runs one command type after CommandExecutor has applied the
// behavior/FSM admission checks and the busy gate.
using Handler = std::function<void(const nlohmann::json&)>;

// Command type -> handler. The registry replaces the previous if/else chain in
// CommandExecutor::robot_bt.
using HandlerRegistry = std::unordered_map<std::string, Handler>;

}  // namespace command
