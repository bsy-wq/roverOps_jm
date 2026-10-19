/*

状态快照，不负责命令调度。
健康状态是准入条件；跟随可抢占导航；上装按独立资源处理。
当前跟随仅为占位，不向底层发送控制指令。

*/


#pragma once
#include <string>

// 系统健康状态
enum class HealthStatus { 
    Unknow,
    Healthy,
    UnHealthy
};

// 当前运动控制模式
enum class MotionMode { 
    Idle,
    StartingNavigation,
    NavigationReady,
    StartingFollow,
    Following,
    Navigating,
    Mapping,
    Stopping,
    Failed
};

// 任务状态
enum class TaskStatus {
    Idle,
    Starting,
    Running,
    Paused,
    Stopping,
    Successed,
    Cancelled,
    Failed
};

// 上装类型
enum class PayloadType { 
    GasDetection,
    AIRecognition,
    HikvisionCamera,
    RoboticArm
};

// 上装状态
enum class PayloadStatus {
    Offline,
    Idle,
    Starting,
    Running,
    Stopping,
    Fault
};

// 配合灯语机器狗状态
enum class LightStatus {
    BmsError,
    LowBattey,
    Navigation,
    Follow,
    Default
};

// 系统总体状态
struct RobotDto {
    HealthStatus health = HealthStatus::Unknow;
    MotionMode motion_mode = MotionMode::Idle;
    TaskStatus task_status = TaskStatus::Idle;
    bool navigation_environment_ready = false;
    bool localization_ready = false;
    bool follow_placeholder = false;
    unsigned long long generation = 0;
    std::string fault_reason;

    LightStatus light_status_ = LightStatus::Default;
};





