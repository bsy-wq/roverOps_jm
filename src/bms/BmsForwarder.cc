#include "BmsForwarder.h"
#include "../include/common/json.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>

#include <atomic>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>


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

}  // namespace

class BmsForwarder::Impl {
public:
    Impl(std::string websocket_url, std::string robot_id)
        : websocket_url_(std::move(websocket_url)), robot_id_(std::move(robot_id)) {}

    void updateBattery(const BatterySnapshot& snapshot) {
        if (!snapshot.received) return;
        {
            std::lock_guard<std::mutex> lock(bms_mutex_);
            latest_soc_ = snapshot.soc_percent;
            has_bms_state_ = true;
        }
        queue_websocket_payload(snapshot.web_payload.dump());
    }

    ~Impl() {
        stop();
    }

    bool start() {
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true)) return true;

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

        std::cout << "[BMS WS] target " << websocket_url_ << std::endl;
        return true;
    }

    void stop() {
        if (!running_.exchange(false, std::memory_order_acq_rel)) {
            return;
        }


        if (ws_work_) {
            ws_work_.reset();
        }
        ws_client_.stop();
        if (ws_thread_.joinable()) {
            ws_thread_.join();
        }
    }

private:
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

        double soc = 0;
        {
            std::lock_guard<std::mutex> lock(bms_mutex_);
            if (!has_bms_state_) {
                return;
            }
            soc = latest_soc_;
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
            {"battery_level", static_cast<int>(soc)},
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

    std::string websocket_url_;
    std::string robot_id_;

    std::mutex bms_mutex_;
    double latest_soc_ = 0;
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

BmsForwarder::BmsForwarder(std::string websocket_url, std::string robot_id)
    : impl_(std::make_unique<Impl>(std::move(websocket_url), std::move(robot_id))) {}

BmsForwarder::~BmsForwarder() = default;

bool BmsForwarder::start() {
    return impl_->start();
}

void BmsForwarder::stop() {
    impl_->stop();
}

void BmsForwarder::updateBattery(const BatterySnapshot& snapshot) {
    impl_->updateBattery(snapshot);
}
