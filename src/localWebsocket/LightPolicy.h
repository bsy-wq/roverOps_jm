#pragma once
#include "robotDto.h"

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
