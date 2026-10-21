#include "BatteryProvider.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>
#include <utility>
#include <vector>
#include <robot_gateway/M20BmsState.h>
#ifdef HAVE_A10_BMS
#include <bms_ros_bridge/BmsStatus.h>
#endif
namespace {
using json = nlohmann::json;
long long unix_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
void add_active_level(json& levels, const char* key, const char* label, int level) {
    if (level <= 0) {
        return;
    }
    levels.push_back({
        {"key", key},
        {"label", label},
        {"level", level}
    });
}

void add_active_flag(json& flags, const char* key, const char* label, bool active) {
    if (!active) {
        return;
    }
    flags.push_back({
        {"key", key},
        {"label", label}
    });
}

std::string join_alarm_labels(const json& active_alarm_levels, const json& active_flags) {
    std::vector<std::string> labels;
    for (const auto& alarm : active_alarm_levels) {
        labels.push_back(alarm.value("label", ""));
    }
    for (const auto& alarm : active_flags) {
        labels.push_back(alarm.value("label", ""));
    }

    std::string result;
    for (const auto& label : labels) {
        if (label.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ",";
        }
        result += label;
    }
    return result;
}

json bms_state_to_json(const robot_gateway::M20BmsState& msg,
                       const std::string& robot_id,
                       const std::string& source_topic) {
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
    add_active_flag(active_flags, "precharge_failed", "预充失败", msg.precharge_failed);
    add_active_flag(active_flags, "parallel_comm_failed", "并联通信失败", msg.parallel_comm_failed);
    add_active_flag(active_flags, "heater_fault", "加热故障", msg.heater_fault);

    const json alarm_flags = json::array({
        msg.cell_overvoltage_level > 0,
        msg.cell_undervoltage_level > 0,
        msg.smart_charger_connected,
        msg.smart_charger_failed,
        msg.cell_diff_over_level > 0,
        msg.chg_high_temp_level > 0,
        msg.smart_load_connected,
        msg.smart_load_failed,
        msg.chg_low_temp_level > 0,
        msg.dsg_high_temp_level > 0,
        msg.chg_mos_overtemp,
        msg.chg_mos_temp_fault,
        msg.dsg_low_temp_level > 0,
        msg.temp_diff_over_level > 0,
        msg.dsg_mos_overtemp,
        msg.dsg_mos_temp_fault,
        msg.total_overvoltage_level > 0,
        msg.total_undervoltage_level > 0,
        msg.short_circuit_protect,
        msg.chg_overcurrent_level > 0,
        msg.dsg_overcurrent_level > 0,
        msg.lv_chg_forbidden,
        msg.hv_dsg_forbidden,
        msg.soc_low_level > 0,
        msg.soh_low_level > 0,
        msg.parallel_comm_success,
        msg.parallel_comm_failed,
        msg.cell_mos_overtemp_level > 0,
        msg.thermal_runaway_level > 0,
        msg.afe_fault,
        msg.afe_comm_fault,
        msg.afe_sample_fault,
        msg.voltage_detect_fault,
        msg.voltage_sampling_line_lost,
        msg.total_voltage_fault,
        msg.current_detect_fault,
        msg.temp_detect_fault,
        msg.temp_sampling_line_lost,
        msg.eeprom_fault,
        msg.flash_fault,
        msg.rtc_fault,
        msg.chg_mos_fault,
        msg.dsg_mos_fault,
        msg.precharge_mos_fault,
        msg.precharge_failed,
        msg.comm_cmd_chg_mos_off,
        msg.comm_cmd_dsg_mos_off,
        msg.switch_chg_mos_off,
        msg.switch_dsg_mos_off,
        msg.fan_working,
        msg.heater_working,
        msg.current_limiter_working,
        msg.heater_fault
    });

    return {
        {"type", "battery_status"},
        {"timestamp", unix_timestamp_ms()},
        {"robot_id", robot_id},
        {"source_topic", source_topic},
        {"voltage_v", msg.voltage_v},
        {"current_a", msg.current_a},
        {"soc_percent", static_cast<int>(msg.soc_percent)},
        {"life", static_cast<int>(msg.life)},
        {"power_w", msg.power_w},
        {"total_energy_wh", msg.total_energy_wh},
        {"mos_temp_c", msg.mos_temp_c},
        {"board_temp_c", msg.board_temp_c},
        {"heat_temp_c", msg.heat_temp_c},
        {"heat_cur_a", msg.heat_cur_a},
        {"bat_state", static_cast<int>(msg.bat_state)},
        {"charge_state", static_cast<int>(msg.charge_state)},
        {"load_detect", static_cast<int>(msg.load_detect)},
        {"do_state", static_cast<int>(msg.do_state)},
        {"di_state", static_cast<int>(msg.di_state)},
        {"cell_overvoltage_level", static_cast<int>(msg.cell_overvoltage_level)},
        {"cell_undervoltage_level", static_cast<int>(msg.cell_undervoltage_level)},
        {"cell_diff_over_level", static_cast<int>(msg.cell_diff_over_level)},
        {"chg_high_temp_level", static_cast<int>(msg.chg_high_temp_level)},
        {"chg_low_temp_level", static_cast<int>(msg.chg_low_temp_level)},
        {"dsg_high_temp_level", static_cast<int>(msg.dsg_high_temp_level)},
        {"dsg_low_temp_level", static_cast<int>(msg.dsg_low_temp_level)},
        {"temp_diff_over_level", static_cast<int>(msg.temp_diff_over_level)},
        {"total_overvoltage_level", static_cast<int>(msg.total_overvoltage_level)},
        {"total_undervoltage_level", static_cast<int>(msg.total_undervoltage_level)},
        {"chg_overcurrent_level", static_cast<int>(msg.chg_overcurrent_level)},
        {"dsg_overcurrent_level", static_cast<int>(msg.dsg_overcurrent_level)},
        {"soc_low_level", static_cast<int>(msg.soc_low_level)},
        {"soh_low_level", static_cast<int>(msg.soh_low_level)},
        {"cell_mos_overtemp_level", static_cast<int>(msg.cell_mos_overtemp_level)},
        {"thermal_runaway_level", static_cast<int>(msg.thermal_runaway_level)},
        {"smart_charger_failed", msg.smart_charger_failed},
        {"smart_load_connected", msg.smart_load_connected},
        {"smart_load_failed", msg.smart_load_failed},
        {"chg_mos_overtemp", msg.chg_mos_overtemp},
        {"chg_mos_temp_fault", msg.chg_mos_temp_fault},
        {"dsg_mos_overtemp", msg.dsg_mos_overtemp},
        {"dsg_mos_temp_fault", msg.dsg_mos_temp_fault},
        {"short_circuit_protect", msg.short_circuit_protect},
        {"lv_chg_forbidden", msg.lv_chg_forbidden},
        {"hv_dsg_forbidden", msg.hv_dsg_forbidden},
        {"afe_fault", msg.afe_fault},
        {"afe_comm_fault", msg.afe_comm_fault},
        {"afe_sample_fault", msg.afe_sample_fault},
        {"voltage_detect_fault", msg.voltage_detect_fault},
        {"voltage_sampling_line_lost", msg.voltage_sampling_line_lost},
        {"total_voltage_fault", msg.total_voltage_fault},
        {"current_detect_fault", msg.current_detect_fault},
        {"temp_detect_fault", msg.temp_detect_fault},
        {"temp_sampling_line_lost", msg.temp_sampling_line_lost},
        {"eeprom_fault", msg.eeprom_fault},
        {"flash_fault", msg.flash_fault},
        {"rtc_fault", msg.rtc_fault},
        {"chg_mos_fault", msg.chg_mos_fault},
        {"dsg_mos_fault", msg.dsg_mos_fault},
        {"precharge_mos_fault", msg.precharge_mos_fault},
        {"precharge_failed", msg.precharge_failed},
        {"parallel_comm_success", msg.parallel_comm_success},
        {"parallel_comm_failed", msg.parallel_comm_failed},
        {"comm_cmd_chg_mos_off", msg.comm_cmd_chg_mos_off},
        {"comm_cmd_dsg_mos_off", msg.comm_cmd_dsg_mos_off},
        {"switch_chg_mos_off", msg.switch_chg_mos_off},
        {"switch_dsg_mos_off", msg.switch_dsg_mos_off},
        {"fan_working", msg.fan_working},
        {"heater_working", msg.heater_working},
        {"current_limiter_working", msg.current_limiter_working},
        {"heater_fault", msg.heater_fault},
        {"smart_charger_connected", msg.smart_charger_connected},
        {"alarm_flags", alarm_flags},
        {"active_alarm_levels", active_alarm_levels},
        {"active_flags", active_flags},
        {"alarm_info", join_alarm_labels(active_alarm_levels, active_flags)}
    };
}


}
class BatteryProvider::Impl {
public:
    Impl(ros::NodeHandle& nh, std::string id, std::string topic, std::string model)
        : nh_(nh), robot_id_(std::move(id)), topic_(std::move(topic)), model_(std::move(model)) {
        std::transform(model_.begin(), model_.end(), model_.begin(),
            [](unsigned char c) { return std::toupper(c); });
        if (model_ == "A10") topic_ = "/bms_status";
        else if (topic_.empty()) topic_ = "/m20/bms_state";
    }
    bool start() {
        if (model_ != "M20" && model_ != "A10") {
            ROS_ERROR_STREAM("[BMS ROS] Unsupported robot.model: " << model_);
            return false;
        }
#ifndef HAVE_A10_BMS
        if (model_ == "A10") {
            ROS_ERROR("[BMS ROS] A10 requires bms_ros_bridge/BmsStatus.h; source its workspace and reconfigure/rebuild");
            return false;
        }
#endif
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            return true;
        }

#ifdef HAVE_A10_BMS
        if (model_ == "A10") {
            subscriber_ = nh_.subscribe(topic_, 10, &Impl::on_a10_bms_state, this);
        } else
#endif
        {
            subscriber_ = nh_.subscribe(
                topic_, 10, &Impl::on_bms_state, this);
        }
        ROS_INFO_STREAM("[BMS ROS] model = " << model_ << " topic = " << topic_);


        return true;
    }
    void stop() {
        running_.store(false);
        subscriber_.shutdown();
    }
    void publish(BatterySnapshot value) {
        if (!running_.load()) return;
        value.received_at = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            latest_ = value;
        }
        if (callback_) callback_(value);
    }
#ifdef HAVE_A10_BMS
    template <typename Battery>
    static json a10_battery_to_json(const Battery& battery) {
        json flags = json::array();
        for (const auto flag : battery.alarm_flags) flags.push_back(bool(flag));
        return {{"voltage_v", battery.voltage_v}, {"current_a", battery.current_a},
                {"temperature_c", battery.temperature_c}, {"soc_percent", int(battery.soc_percent)},
                {"cycle_count", battery.cycle_count}, {"soh_percent", int(battery.soh_percent)},
                {"active_alarm_count", battery.active_alarm_count}, {"bat_state", int(battery.bat_state)},
                {"chg_detect", int(battery.chg_detect)}, {"charge_mode", int(battery.charge_mode)},
                {"alarm_flags", flags}};
    }

    void on_a10_bms_state(const bms_ros_bridge::BmsStatus::ConstPtr& msg) {
        if (!msg) return;
        const bool present1 = msg->bat01.voltage_v > 0;
        const bool present2 = msg->bat02.voltage_v > 0;
        double soc = 0;
        if (present1 && present2) soc = std::min(double(msg->bat01.soc_percent), double(msg->bat02.soc_percent));
        else if (present1) soc = msg->bat01.soc_percent;
        else if (present2) soc = msg->bat02.soc_percent;
        const bool fault = msg->bat01.active_alarm_count > 0 || msg->bat02.active_alarm_count > 0;
        const json battery = {
            {"type", "battery_status"}, {"timestamp", unix_timestamp_ms()},
            {"robot_id", robot_id_}, {"model", "A10"}, {"source_topic", topic_},
            {"soc_percent", soc}, {"bat01", a10_battery_to_json(msg->bat01)},
            {"bat02", a10_battery_to_json(msg->bat02)}, {"current_peri", msg->current_peri},
            {"current_leg1", msg->current_leg1}, {"current_leg2", msg->current_leg2},
            {"current_leg3", msg->current_leg3}, {"current_leg4", msg->current_leg4},
            {"source_timestamp", msg->timestamp}, {"charge_result", int(msg->charge_result)}};
        BatterySnapshot snapshot;
        snapshot.received = true;
        snapshot.valid = present1 || present2;
        snapshot.fault = fault || !snapshot.valid;
        snapshot.soc_percent = soc;
        snapshot.web_payload = battery;
        snapshot.status_info = battery;
        for (const char* key : {"type", "timestamp", "robot_id", "source_topic"}) snapshot.status_info.erase(key);
        publish(std::move(snapshot));
    }
#endif

    void on_bms_state(const robot_gateway::M20BmsState::ConstPtr& msg) {
        if (!msg) {
            return;
        }


        const json battery = bms_state_to_json(*msg, robot_id_, topic_);
        bool fault = msg->precharge_mos_fault || !battery["active_flags"].empty();
        for (const auto& alarm : battery["active_alarm_levels"]) {
            // SOC low is handled by the configurable battery threshold.
            if (alarm["key"] != "soc_low_level") fault = true;
        }
        BatterySnapshot snapshot;
        snapshot.received = true;
        snapshot.valid = true;
        snapshot.fault = fault;
        snapshot.soc_percent = msg->soc_percent;
        snapshot.web_payload = battery;
        for (const char* key : {"voltage_v", "soc_percent", "active_alarm_levels", "active_flags"})
            snapshot.status_info[key] = battery[key];
        publish(std::move(snapshot));
    }


    ros::NodeHandle nh_;
    std::string robot_id_, topic_, model_;
    ros::Subscriber subscriber_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    BatterySnapshot latest_;
    Callback callback_;
};
BatteryProvider::BatteryProvider(ros::NodeHandle& nh, std::string id, std::string topic, std::string model)
    : impl_(std::make_unique<Impl>(nh, std::move(id), std::move(topic), std::move(model))) {}
BatteryProvider::~BatteryProvider() { stop(); }
bool BatteryProvider::start() { return impl_->start(); }
void BatteryProvider::stop() { impl_->stop(); }
void BatteryProvider::setCallback(Callback callback) { impl_->callback_ = std::move(callback); }
BatterySnapshot BatteryProvider::snapshot() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->latest_;
}
