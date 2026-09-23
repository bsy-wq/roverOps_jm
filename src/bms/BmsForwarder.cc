#include "BmsForwarder.h"
#include "../include/common/json.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <robot_gateway/M20BmsState.h>

namespace {

using json = nlohmann::json;
using BmsWebSocketClient = websocketpp::client<websocketpp::config::asio>;
using BmsWebSocketWorkGuard =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

constexpr int kReconnectDelayMs = 5000;
constexpr int kRobotStatusIntervalMs = 2000;

long long unix_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

long long unix_timestamp_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(
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

}  // namespace

class BmsForwarder::Impl {
public:
    Impl(ros::NodeHandle& nh, std::string websocket_url,
         std::string robot_id, std::string bms_topic)
        : nh_(nh),
          websocket_url_(std::move(websocket_url)),
          robot_id_(std::move(robot_id)),
          bms_topic_(bms_topic.empty() ? "/m20/bms_state" : std::move(bms_topic)) {}

    ~Impl() {
        stop();
    }

    bool start() {
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            return true;
        }

        bms_subscriber_ = nh_.subscribe(
            bms_topic_, 10, &Impl::on_bms_state, this);

        ws_client_.init_asio();
        ws_client_.clear_access_channels(websocketpp::log::alevel::all);
        ws_client_.clear_error_channels(websocketpp::log::elevel::all);
        ws_client_.set_open_handler(
            [this](websocketpp::connection_hdl hdl) { on_websocket_open(hdl); });
        ws_client_.set_close_handler(
            [this](websocketpp::connection_hdl hdl) { on_websocket_close(hdl); });
        ws_client_.set_fail_handler(
            [this](websocketpp::connection_hdl hdl) { on_websocket_fail(hdl); });

        ws_work_ = std::make_unique<BmsWebSocketWorkGuard>(
            ws_client_.get_io_service().get_executor());
        ws_thread_ = std::thread([this]() {
            ws_client_.run();
        });

        boost::asio::post(ws_client_.get_io_service(), [this]() {
            connect_websocket();
        });

        // std::cout << "[BMS ROS] subscribed to " << bms_topic_ << std::endl;
        std::cout << "[BMS WS] target " << websocket_url_ << std::endl;
        return true;
    }

    void stop() {
        if (!running_.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        bms_subscriber_.shutdown();

        if (ws_work_) {
            ws_work_.reset();
        }
        ws_client_.stop();
        if (ws_thread_.joinable()) {
            ws_thread_.join();
        }
    }

private:
    void on_bms_state(const robot_gateway::M20BmsState::ConstPtr& msg) {
        if (!msg) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(bms_mutex_);
            latest_bms_state_ = *msg;
            has_bms_state_ = true;
        }

        const std::string payload =
            bms_state_to_json(*msg, robot_id_, bms_topic_).dump();
        // std::cout << "[BMS ROS] update topic=" << bms_topic_
        //           << " voltage_v=" << msg->voltage_v
        //           << " current_a=" << msg->current_a
        //           << " soc_percent=" << static_cast<int>(msg->soc_percent)
        //           << std::endl;
        queue_websocket_payload(payload);
    }

    void queue_websocket_payload(const std::string& payload) {
        {
            std::lock_guard<std::mutex> lock(payload_mutex_);
            pending_payload_ = payload;
        }

        if (!running_.load(std::memory_order_acquire)) {
            return;
        }
        boost::asio::post(ws_client_.get_io_service(), [this]() {
            send_latest_payload();
        });
    }

    void connect_websocket() {
        if (!running_.load(std::memory_order_acquire) ||
            websocket_connected_ || websocket_connecting_) {
            return;
        }

        websocket_connecting_ = true;
        websocketpp::lib::error_code ec;
        const auto connection = ws_client_.get_connection(websocket_url_, ec);
        if (ec) {
            websocket_connecting_ = false;
            std::cerr << "[BMS WS] get_connection failed: "
                      << ec.message() << std::endl;
            schedule_reconnect();
            return;
        }

        websocket_hdl_ = connection->get_handle();
        ws_client_.connect(connection);
    }

    void on_websocket_open(websocketpp::connection_hdl hdl) {
        websocket_hdl_ = hdl;
        websocket_connected_ = true;
        websocket_connecting_ = false;
        reconnect_scheduled_ = false;
        const uint64_t connection_generation = ++websocket_generation_;
        // std::cout << "[BMS WS] connected; robot_status interval="
        //           << kRobotStatusIntervalMs << " ms" << std::endl;
        send_latest_payload();
        send_robot_status();
        schedule_robot_status(connection_generation);
    }

    void on_websocket_close(websocketpp::connection_hdl) {
        websocket_connected_ = false;
        websocket_connecting_ = false;
        // std::cerr << "[BMS WS] closed" << std::endl;
        schedule_reconnect();
    }

    void on_websocket_fail(websocketpp::connection_hdl hdl) {
        websocket_connected_ = false;
        websocket_connecting_ = false;

        websocketpp::lib::error_code ec;
        const auto connection = ws_client_.get_con_from_hdl(hdl, ec);
        if (!ec && connection) {
            std::cerr << "[BMS WS] connection failed: "
                      << connection->get_ec().message() << std::endl;
        } else {
            std::cerr << "[BMS WS] connection failed" << std::endl;
        }
        schedule_reconnect();
    }

    void send_latest_payload() {
        if (!websocket_connected_) {
            return;
        }

        std::string payload;
        {
            std::lock_guard<std::mutex> lock(payload_mutex_);
            payload = pending_payload_;
        }
        if (payload.empty()) {
            return;
        }

        websocketpp::lib::error_code ec;
        ws_client_.send(websocket_hdl_, payload,
                        websocketpp::frame::opcode::text, ec);
        if (ec) {
            websocket_connected_ = false;
            std::cerr << "[BMS WS] battery_status send failed: "
                      << ec.message() << std::endl;
            schedule_reconnect();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(payload_mutex_);
            if (pending_payload_ == payload) {
                pending_payload_.clear();
            }
        }
        // std::cout << "[BMS WS] sent battery_status: " << payload << std::endl;
    }

    void send_robot_status() {
        if (!websocket_connected_) {
            return;
        }

        robot_gateway::M20BmsState bms_state;
        {
            std::lock_guard<std::mutex> lock(bms_mutex_);
            if (!has_bms_state_) {
                return;
            }
            bms_state = latest_bms_state_;
        }

        const json robot_status = {
            {"type", "robot_status"},
            {"timestamp", unix_timestamp_s()},
            {"robot_id", robot_id_},
            {"map_id", "1"},
            {"current_mode", "auto"},
            {"emergency_stop", false},
            {"action", "stop"},
            {"cur_travel_distance", 0.0},
            {"total_travel_distance", 0.0},
            {"localize_status", 0},
            {"relocalize_status", 0},
            {"battery_level", static_cast<int>(bms_state.soc_percent)},
            {"obd_status", 255},
            {"linear_velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
            {"angular_velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
            {"position", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
            {"orientation", {{"roll", 0.0}, {"pitch", 0.0}, {"yaw", 0.0}}},
            {"error", {
                {"error_code", "None"},
                {"error_message", "No error"}
            }}
        };

        websocketpp::lib::error_code ec;
        const std::string payload = robot_status.dump();
        ws_client_.send(websocket_hdl_, payload,
                        websocketpp::frame::opcode::text, ec);
        if (ec) {
            websocket_connected_ = false;
            std::cerr << "[BMS WS] robot_status send failed: "
                      << ec.message() << std::endl;
            schedule_reconnect();
            return;
        }
        // std::cout << "[BMS WS] sent robot_status: " << payload << std::endl;
    }

    void schedule_robot_status(uint64_t connection_generation) {
        ws_client_.set_timer(
            kRobotStatusIntervalMs,
            [this, connection_generation](const websocketpp::lib::error_code& ec) {
                if (ec ||
                    !running_.load(std::memory_order_acquire) ||
                    !websocket_connected_ ||
                    connection_generation != websocket_generation_) {
                    return;
                }

                send_robot_status();
                if (websocket_connected_) {
                    schedule_robot_status(connection_generation);
                }
            });
    }

    void schedule_reconnect() {
        if (!running_.load(std::memory_order_acquire) || reconnect_scheduled_) {
            return;
        }

        reconnect_scheduled_ = true;
        ws_client_.set_timer(
            kReconnectDelayMs,
            [this](const websocketpp::lib::error_code& ec) {
                reconnect_scheduled_ = false;
                if (!ec && running_.load(std::memory_order_acquire)) {
                    connect_websocket();
                }
            });
    }

    ros::NodeHandle nh_;
    std::string websocket_url_;
    std::string robot_id_;
    std::string bms_topic_;

    ros::Subscriber bms_subscriber_;
    std::mutex bms_mutex_;
    robot_gateway::M20BmsState latest_bms_state_;
    bool has_bms_state_ = false;

    std::atomic_bool running_{false};
    std::thread ws_thread_;

    BmsWebSocketClient ws_client_;
    std::unique_ptr<BmsWebSocketWorkGuard> ws_work_;
    websocketpp::connection_hdl websocket_hdl_;
    bool websocket_connected_ = false;
    bool websocket_connecting_ = false;
    bool reconnect_scheduled_ = false;
    uint64_t websocket_generation_ = 0;

    std::mutex payload_mutex_;
    std::string pending_payload_;
};

BmsForwarder::BmsForwarder(ros::NodeHandle& nh, std::string websocket_url,
                           std::string robot_id, std::string bms_topic)
    : impl_(std::make_unique<Impl>(
          nh, std::move(websocket_url), std::move(robot_id),
          std::move(bms_topic))) {}

BmsForwarder::~BmsForwarder() = default;

bool BmsForwarder::start() {
    return impl_->start();
}

void BmsForwarder::stop() {
    impl_->stop();
}
