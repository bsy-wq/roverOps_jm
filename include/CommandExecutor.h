#pragma once

#include "../include/common/json.hpp"
#include "../include/common/utils.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <ros/ros.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseArray.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <nav_msgs/Path.h>
#include <rosgraph_msgs/Log.h>
#include <signal.h>
#include <sys/types.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/Int8.h>
#include <std_msgs/String.h>
#include <tf/transform_listener.h>
#include <pct_planner/MultiWaypointNavigationAction.h>
#include <actionlib/client/simple_action_client.h>
#include <tf/transform_datatypes.h>
#include <curl/curl.h>
#include "mapping/maphandle.h"
#include "hikvision/hikvihandle.h"
#include "robotState/robotState.h"
#include "modifyValue/modifyHandle.h"
#include "motionControl/motionHandle.h"

using json = nlohmann::json;

#define MAP_MANAGER_LOCAL "/home/jianmi/map_manager_local/"

class gateway;
class MapManager;

class CommandExecutor {
public:
    CommandExecutor() = default;
    CommandExecutor(ros::NodeHandle& nh,
                    const std::string& device_id, const std::string& http_ip,
                    const int& http_port,
                    const std::string& scripts_path, const std::string& map_dir,
                    const std::vector<std::string>& map_files, const char* bms_model,
                    tinyxml2::XMLDocument& doc, const std::string& parameter_path, const std::string& parameter_yaml_path);
    ~CommandExecutor();

    void ros_prepare(const json& payload_json);
    void execute(const json& payload_json);
    void send_to_cloud(const std::string& msg);
    void try_flush_msgs();
    void setGateway(gateway* gw);

    void handle_initial_pose(const json& data);
    void handle_start_navigation(const json& data);
    void handle_navigation_task(const json& data);
    void handle_set_dock_pose(const json& data);
    void handle_stop_navigation(const json& data);
    void handle_start_mapping(const json& data);
    void handle_stop_and_uploadmap(const json& data);
    void handle_start_sh(const json& data);
    void start_navigation_async(json data, std::string uuid_old, std::string uuid_new);
    void stop_and_uploadmap_async(json data);
    std::string get_last_storage_path() const;
    void set_upload_enable(bool enable);
    bool get_localization_ready() const;
    std::string get_current_map_id() const;
    void set_current_map_id(const std::string& id);
    std::string get_state_name() const;
    void loc_callback(const std_msgs::Int8::ConstPtr& msg);
    bool get_robot_position(double& x, double& y, double& z);
    bool get_robot_orientation(double& roll, double& pitch, double& yaw);
    void exec_upload_cmd(const json& data);

    void doneCb(const actionlib::SimpleClientGoalState &state,
                const pct_planner::MultiWaypointNavigationResultConstPtr &result);
    void feedbackCb(const pct_planner::MultiWaypointNavigationFeedbackConstPtr &feedback);

    void start_add_task();
    void start_add_point();
    void stop_add_point();
    void stop_add_task();
    void exec_rosservice_cmd(const std::string& type, const std::string& service_name);

    void add_upload_sn(std::string sn, std::string http_ip, int http_port) {
        // std::string url = "http://58.240.76.186:2800/sn_add?sn=" + sn;
        std::string url = "http://" + http_ip + ":" + std::to_string(http_port) + "/sn_add?sn=" + sn;
        CURL* curl = curl_easy_init();

        if (!curl) {
            std::cout << "curl init failed" << std::endl;
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](char* ptr, size_t size, size_t nmemb, std::string* data) -> size_t {
                data->append(ptr, size * nmemb);
                return size * nmemb;
            });

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cout << "request failed: "
                    << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "response: " << response << std::endl;
        }

        curl_easy_cleanup(curl);
    }

private:
    void init_ros_publishers();
    void init_ros_subscribers();
    void global_path_callback(const nav_msgs::Path::ConstPtr& msg);
    void localization_callback(const nav_msgs::Odometry::ConstPtr& msg);
    void initial_pose_rosout_callback(const rosgraph_msgs::Log::ConstPtr& msg);
    void complete_initial_pose_result(unsigned long long attempt_id, bool success, const std::string& message);
    void complete_navigation_env_ready(unsigned long long attempt_id, bool success, const std::string& message);
    void complete_navigation_task_result(unsigned long long attempt_id, bool success, const std::string& message);
    void complete_clear_waypoints_result(unsigned long long attempt_id, bool success, const std::string& message);
    void handle_navigation_planning_rosout(const std::string& message);
    void send_navigation_planning_failure();
    bool try_begin_command(const std::string& cmd_type);
    void finish_command(const std::string& cmd_type);
    bool is_managed_command(const std::string& cmd_type) const;
    void enqueue_task_state_request(const std::string& request_type, bool target_paused);
    void process_next_task_state_request();
    void complete_task_state_transition(bool success, int ret);
    void telemetry_loop();
    void handle_set_position_upload_frequency(const json& data);
    bool should_send_upload_position();
    bool is_pid_still_running(pid_t pid) const;
    bool is_navigation_environment_running(std::string& reason);
    void launch_background_task(std::thread&& worker);
    void log_runtime_flags(const char* stage) const;
    void log_wait_status(const std::string& name, int status) const;
    bool log_file_access(const std::string& name, const std::string& path, int mode) const;
    std::string shell_quote(const std::string& value) const;
    std::string script_log_command(const std::string& script_path, const std::string& tag) const;

private:
    ros::NodeHandle nh_;
    std::string ws_url_;
    std::string device_id_;
    std::string http_ip_;
    int http_port_;
    std::string bms_topic_ = "/m20/bms_state";
    std::string scripts_path_;
    std::string map_dir_;
    std::vector<std::string> map_files_;
    tinyxml2::XMLDocument& doc_;
    std::string parameter_path_;
    std::string parameter_yaml_path_;
    std::unique_ptr<MapManager> map_manager_;
    std::unique_ptr<HikvisionHandle> hikvision_handle_;
    ModifyHandle modify_handle_;
    std::unique_ptr<MotionHandle> motion_handle_;

    ros::Publisher initial_pose_pub_;
    ros::Publisher loops_pub_;
    ros::Publisher goals_pub_;
    ros::Publisher wait_times_pub_;
    ros::Publisher control_pub_;
    ros::Publisher goal_pub_;
    ros::Publisher dock_pose_pub_;
    ros::Publisher cycles_pub_;
    ros::Publisher neupanPathPub;

    ros::Subscriber loc_sub_;
    ros::Subscriber global_path_sub_;
    ros::Subscriber rosout_sub_;
    ros::Subscriber robot_state_;
    ros::Subscriber bms_info_;

    tf::TransformListener tf_listener_;

    std::atomic<bool> navigation_env_ready_{false};
    std::atomic<bool> localization_system_ready_{false};
    std::atomic<bool> upload_enable_{false};
    std::atomic<bool> localization_ready_{false};
    std::string current_map_id_ = "map_001";
    pid_t nav_process_ = -1;
    pid_t goal_process_ = -1;
    pid_t mapping_process_ = -1;
    pid_t cleanup_process_ = -1;

    std::unique_ptr<robotState> robot_state_handle_;

    mutable std::mutex process_mutex_;
    std::unique_ptr<MappingHandle> mapping_handle_;
    mutable std::mutex state_mutex_;
    std::mutex command_state_mutex_;
    std::mutex task_state_mutex_;
    std::mutex worker_mutex_;
    std::mutex initial_pose_mutex_;
    std::mutex navigation_env_mutex_;
    std::mutex navigation_task_mutex_;
    std::mutex clear_waypoints_mutex_;
    std::mutex navigation_planning_failure_mutex_;
    std::vector<std::thread> background_workers_;
    std::thread telemetry_thread_;
    std::mutex msg_mutex_;
    std::deque<std::string> pending_msgs_;
    gateway* gateway_ = nullptr;
    std::mutex task_execution_progress_mutex_;
    std::string robot_model_ = "M20";
    std::atomic<bool> stop_upload_running_{false};
    std::atomic<long long> last_upload_position_ms_{0};
    std::atomic<long long> upload_position_interval_ms_{1000};
    static constexpr int kInitialPoseTimeoutSec_ = 20;
    static constexpr int kNavigationEnvReadyTimeoutSec_ = 120;
    static constexpr int kNavigationTaskAckTimeoutSec_ = 10;
    static constexpr int kClearWaypointsAckTimeoutSec_ = 10;
    bool initial_pose_pending_ = false;
    unsigned long long initial_pose_attempt_id_ = 0;
    bool navigation_env_pending_ = false;
    unsigned long long navigation_env_attempt_id_ = 0;
    bool navigation_task_pending_ = false;
    unsigned long long navigation_task_attempt_id_ = 0;
    size_t navigation_task_expected_points_ = 0;
    size_t navigation_task_confirmed_points_ = 0;
    bool clear_waypoints_pending_ = false;
    unsigned long long clear_waypoints_attempt_id_ = 0;
    bool navigation_planning_failure_pending_ = false;
    bool task_execution_active_ = false;
    std::string navigation_planning_fault_position_;
    std::string navigation_planning_failure_reason_;
    double navigation_planning_target_x_ = 0.0;
    double navigation_planning_target_y_ = 0.0;
    double navigation_planning_target_z_ = 0.0;
    size_t task_execution_total_points_ = 0;
    size_t task_execution_reached_points_ = 0;
    bool navigation_planning_target_valid_ = false;
    std::string active_command_;
    struct TaskStateRequest {
        std::string request_type;
        bool target_paused;
    };
    std::deque<TaskStateRequest> task_state_requests_;
    bool task_state_worker_running_ = false;
    bool task_state_transition_active_ = false;
    bool task_paused_ = false;
    TaskStateRequest active_task_state_request_{"", false};
    pct_planner::MultiWaypointNavigationGoal current_goal_;
    int action_timeout_sec_;
};
