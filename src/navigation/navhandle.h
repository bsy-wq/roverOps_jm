#pragma once

#include "../include/common/json.hpp"

using json = nlohmann::json;

class NvaManager {
public:



    void handle_start_navigation(const json& data);
    void start_navigation_async(json data, std::string uuid_old, std::string uuid);

private:

};