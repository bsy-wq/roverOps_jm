#pragma once

#include <string>
#include "../include/CommandExecutor.h"
#include "../include/common/log_manager.h"

class AiRecognitionAckHandler {
public:
    AiRecognitionAckHandler(CommandExecutor& executor, std::string device_id, std::string image_directory);

    void handle(const std::string& payload, bool binary) const;

private:
    std::string device_id_;
    std::string image_directory_;

    CommandExecutor& executor_;
};
