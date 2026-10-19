#pragma once

#include "../src/robotDto.h"
#include <mutex>

// Owns behavior state; execution and device feedback stay in the adapters.
class RobotFsm {
public:
    enum class FollowDecision { Rejected, AlreadyFollowing, Start };
    RobotDto snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    bool begin_navigation() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Idle ||
            state_.health == HealthStatus::UnHealthy) return false;
        ++state_.generation;
        state_.motion_mode = MotionMode::StartingNavigation;
        state_.task_status = TaskStatus::Idle;
        state_.fault_reason.clear();
        return true;
    }

    bool navigation_current(unsigned long long generation) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return generation == state_.generation && is_navigation(state_.motion_mode);
    }

    void navigation_ready(bool success, const std::string& reason) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::StartingNavigation) return;
        state_.navigation_environment_ready = success;
        state_.motion_mode = success ? MotionMode::NavigationReady : MotionMode::Failed;
        state_.fault_reason = success ? "" : reason;
    }

    FollowDecision begin_follow() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.health == HealthStatus::UnHealthy ||
            state_.motion_mode == MotionMode::Mapping ||
            state_.motion_mode == MotionMode::Stopping ||
            state_.motion_mode == MotionMode::StartingFollow ||
            state_.motion_mode == MotionMode::Failed) return FollowDecision::Rejected;
        if (state_.motion_mode == MotionMode::Following) return FollowDecision::AlreadyFollowing;
        ++state_.generation;
        state_.motion_mode = MotionMode::Stopping;
        state_.task_status = TaskStatus::Stopping;
        return FollowDecision::Start;
    }

    bool begin_stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode == MotionMode::Following || state_.motion_mode == MotionMode::Mapping ||
            state_.motion_mode == MotionMode::Stopping || state_.motion_mode == MotionMode::StartingFollow)
            return false;
        ++state_.generation;
        state_.motion_mode = MotionMode::Stopping;
        state_.task_status = TaskStatus::Stopping;
        return true;
    }

    void stopped(bool success, const std::string& reason, bool follow = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.motion_mode = success ? (follow ? MotionMode::StartingFollow : MotionMode::Idle)
                                     : MotionMode::Failed;
        state_.task_status = success ? TaskStatus::Cancelled : TaskStatus::Failed;
        state_.navigation_environment_ready = false;
        state_.localization_ready = false;
        state_.fault_reason = success ? "" : reason;
    }

    void follow_started() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::StartingFollow) return;
        state_.motion_mode = MotionMode::Following;
        state_.task_status = TaskStatus::Running;
        state_.follow_placeholder = true;
    }

    bool stop_follow() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Following) return false;
        ++state_.generation;
        state_.motion_mode = MotionMode::Idle;
        state_.task_status = TaskStatus::Cancelled;
        state_.follow_placeholder = false;
        return true;
    }

    void task_state(TaskStatus status) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!is_navigation(state_.motion_mode)) return;
        state_.task_status = status;
        state_.motion_mode = (status == TaskStatus::Running || status == TaskStatus::Paused)
            ? MotionMode::Navigating : MotionMode::NavigationReady;
    }

    void localization_ready(bool ready) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_navigation(state_.motion_mode)) state_.localization_ready = ready;
    }

    void mapping(bool active) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.motion_mode = active ? MotionMode::Mapping : MotionMode::Idle;
    }

    bool begin_mapping() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Idle || state_.health == HealthStatus::UnHealthy)
            return false;
        state_.motion_mode = MotionMode::Mapping;
        ++state_.generation;
        return true;
    }

    static bool is_navigation(MotionMode mode) {
        return mode == MotionMode::StartingNavigation || mode == MotionMode::NavigationReady ||
               mode == MotionMode::Navigating;
    }

    static const char* name(MotionMode mode) {
        switch (mode) {
        case MotionMode::Idle: return "Idle";
        case MotionMode::StartingNavigation: return "StartingNavigation";
        case MotionMode::NavigationReady: return "NavigationReady";
        case MotionMode::Navigating: return "Navigating";
        case MotionMode::StartingFollow: return "StartingFollow";
        case MotionMode::Following: return "Following";
        case MotionMode::Mapping: return "Mapping";
        case MotionMode::Stopping: return "StoppingNavigation";
        case MotionMode::Failed: return "Fault";
        }
        return "Unknown";
    }

    static const char* task_name(TaskStatus status) {
        switch (status) {
        case TaskStatus::Idle: return "Idle";
        case TaskStatus::Starting: return "Starting";
        case TaskStatus::Running: return "Running";
        case TaskStatus::Paused: return "Paused";
        case TaskStatus::Stopping: return "Stopping";
        case TaskStatus::Successed: return "Succeeded";
        case TaskStatus::Cancelled: return "Cancelled";
        case TaskStatus::Failed: return "Failed";
        }
        return "Unknown";
    }

private:
    mutable std::mutex mutex_;
    RobotDto state_;
};
