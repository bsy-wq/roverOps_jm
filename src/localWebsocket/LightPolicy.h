#pragma once
#include "robotDto.h"
#include <string>
#include <vector>

inline std::vector<std::string> selectLightValues(
    bool has_bms, bool bms_error, double soc, double threshold, MotionMode mode,
    bool estop, bool lie_down, bool http_follow) {
    std::vector<std::string> values;
    if (estop) values.emplace_back("estop");
    if (has_bms && bms_error) values.emplace_back("bms_error");
    if (has_bms && soc < threshold) values.emplace_back("low_battery");
    if (mode == MotionMode::StartingNavigation || mode == MotionMode::NavigationReady ||
        mode == MotionMode::Navigating) values.emplace_back("navigation");
    if (http_follow || mode == MotionMode::StartingFollow || mode == MotionMode::Following)
        values.emplace_back("follow");
    if (lie_down) values.emplace_back("lie_down");
    values.emplace_back("default");
    return values;
}

inline LightStatus selectLightStatus(bool has_bms, bool bms_error, double soc,
                                    double threshold, MotionMode mode) {
    if (has_bms && bms_error) return LightStatus::BmsError;
    if (has_bms && soc < threshold) return LightStatus::LowBattey;
    if (mode == MotionMode::StartingFollow || mode == MotionMode::Following)
        return LightStatus::Follow;
    if (mode == MotionMode::StartingNavigation || mode == MotionMode::NavigationReady ||
        mode == MotionMode::Navigating) return LightStatus::Navigation;
    return LightStatus::Default;
}

inline const char* lightValue(LightStatus status) {
    switch (status) {
    case LightStatus::BmsError: return "bms_error";
    case LightStatus::LowBattey: return "low_battery";
    case LightStatus::Navigation: return "navigation";
    case LightStatus::Follow: return "follow";
    case LightStatus::Default: return "Default";
    }
    return "Default";
}
