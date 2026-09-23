/*

The feature was designed on September 17, 2026; send bms、robot_state、navigation_env_active、map by frequency of 5 Hz.

*/

#include "robotState.h"

#include <chrono>
#include <utility>

namespace {

using json = nlohmann::json;

long long unix_timestamp_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void add_active_level(json& levels, const char* key, const char* label, int level)
{
    if (level <= 0) {
        return;
    }
    levels.push_back({
        {"key", key},
        {"label", label},
        {"level", level}
    });
}

void add_active_flag(json& flags, const char* key, const char* label, int active)
{
    if (!active) {
        return;
    }
    flags.push_back({
        {"key", key},
        {"label", label}
    });
}

json bms_state_to_json(const robot_gateway::M20BmsState& msg)
{
    json active_alarm_levels = json::array();
    add_active_level(active_alarm_levels, "cell_overvoltage_level", "单体过压告警", msg.cell_overvoltage_level);
    add_active_level(active_alarm_levels, "cell_undervoltage_level", "单体欠压告警", msg.cell_undervoltage_level);
    add_active_level(active_alarm_levels, "cell_diff_over_level", "压差过大告警", msg.cell_diff_over_level);
    add_active_level(active_alarm_levels, "chg_high_temp_level", "充电高温告警", msg.chg_high_temp_level);
    add_active_level(active_alarm_levels, "chg_low_temp_level", "充电低温告警", msg.chg_low_temp_level);
    add_active_level(active_alarm_levels, "dsg_high_temp_level", "放电高温告警", msg.dsg_high_temp_level);
    add_active_level(active_alarm_levels, "dsg_low_temp_level", "放电低温告警", msg.dsg_low_temp_level);
    add_active_level(active_alarm_levels, "temp_diff_over_level", "温差过大告警", msg.temp_diff_over_level);
    add_active_level(active_alarm_levels, "total_overvoltage_level", "总压过高告警", msg.total_overvoltage_level);
    add_active_level(active_alarm_levels, "total_undervoltage_level", "总压过低告警", msg.total_undervoltage_level);
    add_active_level(active_alarm_levels, "chg_overcurrent_level", "充电过流告警", msg.chg_overcurrent_level);
    add_active_level(active_alarm_levels, "dsg_overcurrent_level", "放电过流告警", msg.dsg_overcurrent_level);
    add_active_level(active_alarm_levels, "soc_low_level", "SOC过低告警", msg.soc_low_level);
    add_active_level(active_alarm_levels, "soh_low_level", "SOH过低告警", msg.soh_low_level);
    add_active_level(active_alarm_levels, "cell_mos_overtemp_level", "MOS温度过高告警", msg.cell_mos_overtemp_level);
    add_active_level(active_alarm_levels, "thermal_runaway_level", "热失控告警", msg.thermal_runaway_level);

    json active_flags = json::array();
    add_active_flag(active_flags, "smart_charger_failed", "智能充电器连接失败", msg.smart_charger_failed);
    add_active_flag(active_flags, "smart_load_failed", "智能放电设备连接失败", msg.smart_load_failed);
    add_active_flag(active_flags, "chg_mos_overtemp", "充电MOS温度过高", msg.chg_mos_overtemp);
    add_active_flag(active_flags, "chg_mos_temp_fault", "充电MOS温度检测故障", msg.chg_mos_temp_fault);
    add_active_flag(active_flags, "dsg_mos_overtemp", "放电MOS温度过高", msg.dsg_mos_overtemp);
    add_active_flag(active_flags, "dsg_mos_temp_fault", "放电MOS温度检测故障", msg.dsg_mos_temp_fault);
    add_active_flag(active_flags, "short_circuit_protect", "短路保护", msg.short_circuit_protect);
    add_active_flag(active_flags, "lv_chg_forbidden", "低压禁止充电", msg.lv_chg_forbidden);
    add_active_flag(active_flags, "hv_dsg_forbidden", "高压禁止放电", msg.hv_dsg_forbidden);
    add_active_flag(active_flags, "afe_fault", "AFE芯片故障", msg.afe_fault);
    add_active_flag(active_flags, "afe_comm_fault", "AFE通信故障", msg.afe_comm_fault);
    add_active_flag(active_flags, "afe_sample_fault", "AFE采样故障", msg.afe_sample_fault);
    add_active_flag(active_flags, "voltage_detect_fault", "电压检测故障", msg.voltage_detect_fault);
    add_active_flag(active_flags, "voltage_sampling_line_lost", "电压采集线掉线", msg.voltage_sampling_line_lost);
    add_active_flag(active_flags, "total_voltage_fault", "总压检测故障", msg.total_voltage_fault);
    add_active_flag(active_flags, "current_detect_fault", "电流检测故障", msg.current_detect_fault);
    add_active_flag(active_flags, "temp_detect_fault", "温度检测故障", msg.temp_detect_fault);
    add_active_flag(active_flags, "temp_sampling_line_lost", "温度采集线掉线", msg.temp_sampling_line_lost);
    add_active_flag(active_flags, "eeprom_fault", "EEPROM故障", msg.eeprom_fault);
    add_active_flag(active_flags, "flash_fault", "Flash故障", msg.flash_fault);
    add_active_flag(active_flags, "rtc_fault", "RTC故障", msg.rtc_fault);
    add_active_flag(active_flags, "chg_mos_fault", "充电MOS故障", msg.chg_mos_fault);
    add_active_flag(active_flags, "dsg_mos_fault", "放电MOS故障", msg.dsg_mos_fault);
    add_active_flag(active_flags, "precharge_mos_fault", "预充MOS故障", msg.precharge_mos_fault);
    add_active_flag(active_flags, "precharge_failed", "预充失败", msg.precharge_failed);
    add_active_flag(active_flags, "parallel_comm_failed", "并联通信失败", msg.parallel_comm_failed);
    add_active_flag(active_flags, "heater_fault", "加热故障", msg.heater_fault);

    return {
        {"voltage_v", msg.voltage_v},
        {"soc_percent", static_cast<int>(msg.soc_percent)},
        {"active_alarm_levels", active_alarm_levels},
        {"active_flags", active_flags}
    };
}

}  // namespace

robotState::robotState(
    ros::NodeHandle& nh,
    std::string device_id,
    SendToCloud send_to_cloud)
    : device_id_(std::move(device_id)),
      send_to_cloud_(std::move(send_to_cloud))
{
    heartbeat_timer_ = nh.createTimer(
        ros::Duration(0.2), // websocket 实时返回消息频率，5hz
        [this](const ros::TimerEvent&) {
            publish_snapshot("heartbeat");
        });
}

robotState::~robotState()
{
    heartbeat_timer_.stop();
}

void robotState::read_ros_state_info_callback(const std_msgs::String::ConstPtr& msg)
{
    if (!msg) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        latest_robot_state_ = msg->data;
        has_robot_state_ = true;
    }
}

void robotState::read_ros_bms_info_callback(const robot_gateway::M20BmsState::ConstPtr& msg)
{
    if (!msg) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        latest_bms_info_ = bms_state_to_json(*msg);
        has_bms_info_ = true;
    }

}

void robotState::update_navigation_environment(
    bool active,
    const std::string& address,
    const std::string& display_name)
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = active;
        if (!address.empty() && !display_name.empty()) {
            map_info_ = {
                {"address", address},
                {"display_name", display_name}
            };
        } else {
            map_info_ = json::object();
        }
    }

    publish_snapshot("navigation_env");
}

void robotState::set_navigation_env_active(bool active)
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = active;
    }

    publish_snapshot("navigation_env");
}

void robotState::clear_navigation_environment()
{
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        navigation_env_active_ = false;
        map_info_ = json::object();
    }

    publish_snapshot("navigation_env");
}

void robotState::publish_snapshot(const std::string& updated_source)
{
    if (!send_to_cloud_) {
        return;
    }

    json payload;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        payload = {
            {"type", "robot_status"},
            {"timestamp", unix_timestamp_ms()},
            {"device_id", device_id_},
            // {"updated_source", updated_source},
            {"has_robot_state", has_robot_state_},
            {"has_bms_info", has_bms_info_},
            {"navigation_env_active", navigation_env_active_},
            {"map", map_info_},
            {"version", "0.1.1"}
        };

        if (has_robot_state_) {
            payload["robot_state"] = latest_robot_state_;
        }
        if (has_bms_info_) {
            payload["bms_info"] = latest_bms_info_;
        }
    }

    send_to_cloud_(payload.dump());
}
