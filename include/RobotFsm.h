#pragma once

#include "../src/robotDto.h"
#include <mutex>

// Owns behavior state; execution and device feedback stay in the adapters.
class RobotFsm {
public:
    enum class FollowDecision { Rejected, AlreadyFollowing, Queued, Start };
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

    // Abort a navigation startup that never reached the environment-running
    // stage. Return to Idle so the operator can retry without an extra stop.
    bool navigation_start_aborted() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::StartingNavigation) return false;
        state_.motion_mode = MotionMode::Idle;
        state_.task_status = TaskStatus::Idle;
        state_.navigation_environment_ready = false;
        state_.localization_ready = false;
        state_.fault_reason.clear();
        return true;
    }

    FollowDecision begin_follow() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.health == HealthStatus::UnHealthy ||
            state_.motion_mode == MotionMode::Mapping ||
            state_.motion_mode == MotionMode::MappingProcessing ||
            state_.motion_mode == MotionMode::Failed) return FollowDecision::Rejected;
        // Already started or starting: duplicates are idempotent, not errors.
        if (state_.motion_mode == MotionMode::StartingFollow ||
            state_.motion_mode == MotionMode::Following) return FollowDecision::AlreadyFollowing;
        // A stop is already in progress; queue the follow request instead of
        // rejecting it.
        if (state_.motion_mode == MotionMode::Stopping) return FollowDecision::Queued;
        ++state_.generation;
        state_.motion_mode = MotionMode::Stopping;
        state_.task_status = TaskStatus::Stopping;
        return FollowDecision::Start;
    }

    bool begin_stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode == MotionMode::Following || state_.motion_mode == MotionMode::Mapping ||
            state_.motion_mode == MotionMode::MappingProcessing ||
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

    void follow_started(bool placeholder = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::StartingFollow) return;
        state_.motion_mode = MotionMode::Following;
        state_.task_status = TaskStatus::Running;
        state_.follow_placeholder = placeholder;
    }

    bool stop_follow() {
        return follow_stopped(true, "");
    }

    // Ends a follow that is running or starting. `success == false` is used for
    // a follow emergency stop and leaves the FSM in Fault.
    bool follow_stopped(bool success, const std::string& reason) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Following &&
            state_.motion_mode != MotionMode::StartingFollow) return false;
        ++state_.generation;
        state_.motion_mode = success ? MotionMode::Idle : MotionMode::Failed;
        state_.task_status = success ? TaskStatus::Cancelled : TaskStatus::Failed;
        state_.follow_placeholder = false;
        state_.navigation_environment_ready = false;
        state_.localization_ready = false;
        state_.fault_reason = success ? "" : reason;
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

    bool mapping(bool active) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active) {
            // begin_mapping() already moved the FSM to Mapping. Do not let a
            // late worker force Mapping from Navigation/Follow/Fault.
            if (state_.motion_mode != MotionMode::Idle &&
                state_.motion_mode != MotionMode::Mapping &&
                state_.motion_mode != MotionMode::MappingProcessing) return false;
            state_.motion_mode = MotionMode::Mapping;
            return true;
        }
        // Only a real mapping state may be cleared to Idle.
        if (state_.motion_mode != MotionMode::Mapping &&
            state_.motion_mode != MotionMode::MappingProcessing) return false;
        state_.motion_mode = MotionMode::Idle;
        return true;
    }

    // Collection finished and the map is being post-processed/uploaded. The
    // robot is no longer collecting, but navigation/follow still must wait.
    bool mapping_processing() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Mapping) return false;
        state_.motion_mode = MotionMode::MappingProcessing;
        return true;
    }

    // A mapping workflow failed after the collection started. Leaving Mapping
    // is what matters: the FSM must not stay in Mapping with no process.
    bool mapping_failed(const std::string& reason) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.motion_mode != MotionMode::Mapping &&
            state_.motion_mode != MotionMode::MappingProcessing) return false;
        ++state_.generation;
        state_.motion_mode = MotionMode::Failed;
        state_.task_status = TaskStatus::Failed;
        state_.fault_reason = reason;
        return true;
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
        case MotionMode::MappingProcessing: return "MappingProcessing";
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
