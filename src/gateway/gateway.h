#pragma once
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/common/asio_ssl.hpp>
#include <boost/asio/strand.hpp>
#include <ros/ros.h>
#include <functional>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <algorithm>
#include <atomic>
#include <future>
#include <mutex>
#include <cstdlib>
#include <deque>
#include <unordered_set>
#include <utility>
#include "../include/common/json.hpp"
#include "../include/common/log_manager.h"

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::client<websocketpp::config::asio> client;
typedef websocketpp::connection_hdl connection_hdl;
typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_t;

using json = nlohmann::json;
using MessageCallback = std::function<void(const json &payload_json)>;

class gateway
{
public:
    enum class GatewayState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Degraded,
        Stopping
    };

    gateway();
    ~gateway();

    bool setup(const std::string &web_url);

    void set_device_id(const std::string& device_id);

    void setMessageCallback(MessageCallback cb);
    void setConnectedCallback(std::function<void()> cb);
    void broadcast_data(const std::string &payload);

    bool isWebConnected() const;
    bool sendToWeb(const std::string& payload);
    std::string get_state_name() const;

private:
    server ws_server;
    client ws_client;

    typedef boost::asio::strand<boost::asio::io_context::executor_type> strand_t;
    std::shared_ptr<strand_t> server_strand;
    std::shared_ptr<strand_t> client_strand;

    connection_hdl hdl_handset;   // 遥控器句柄 this->ws_server 连接
    connection_hdl hdl_webserver; // web 服务器句柄 this->ws_client 连接
    connection_hdl hdl_heartbeat; // robotDog 心跳专用连接

    std::atomic<bool> handset_connected{false};
    std::atomic<bool> webserver_connected{false};
    std::atomic<bool> heartbeat_connected{false};
    std::chrono::steady_clock::time_point last_web_packet_time;
    std::chrono::steady_clock::time_point last_web_pong_time;

    std::thread client_thread;
    std::thread::id client_thread_id;
    std::thread server_thread;
    std::unique_ptr<work_guard_t> client_work;

    std::string webURL;
    std::string heartbeatURL;
    std::string device_id_;
    std::string web_host_;
    std::string web_port_;
    int port;
    int pingSecond = 2;
    int pongTimeOut = 5;
    int command_timeout_ms_ = 60000;

    // 重连退避参数
    int reconnect_attempts_ = 0;
    int kicked_reconnect_attempts_ = 0;
    int reconnect_max_attempts_ = 0;        // 0表示无限重连
    int reconnect_base_delay_ms_ = 1000;     // 第一次重连延迟(ms)，默认1秒
    int reconnect_max_delay_ms_ = 60000;     // 最大延迟(ms)，默认60秒
    std::atomic<bool> reconnect_scheduled_{false};
    std::atomic<bool> stopping_{false};
    std::atomic<bool> client_connecting_{false};
    std::atomic<bool> network_diagnostic_running_{false};
    std::atomic<bool> kicked_by_new_default_connection_{false};
    std::atomic<int> connection_generation_{0};

    ros::NodeHandle private_nh_;
    MessageCallback message_callback_;
    std::function<void()> connected_callback_;
    GatewayState state_ = GatewayState::Disconnected;
    mutable std::mutex state_mutex_;
    std::mutex callback_mutex_;
    std::mutex dedup_mutex_;
    std::deque<std::string> recent_command_ids_;
    std::unordered_set<std::string> recent_command_id_set_;
    static constexpr size_t kRecentCommandCacheLimit_ = 100;

    void on_server_open(connection_hdl hdl);
    void on_server_close(connection_hdl hdl);
    void on_client_open(connection_hdl hdl);
    void on_client_close(connection_hdl hdl);
    void on_client_fail(connection_hdl hdl);
    void on_pong(connection_hdl hdl, std::string payload);
    void on_pong_timeout(connection_hdl hdl, std::string payload);
    void on_message(connection_hdl hdl, server::message_ptr msg);

    void connect_to_web();
    void connect_heartbeat();
    void schedule_reconnect();
    void schedule_heartbeat(int generation);
    bool close_web_connection(const char* reason);
    bool is_same_hdl(connection_hdl lhs, connection_hdl rhs) const;
    bool send_to_web_now(const std::string& payload);
    bool send_heartbeat_now(const std::string& payload);
    void mark_web_disconnected(const char* reason, bool reconnect);
    void run_network_diagnostics(const char* reason);
    void update_web_endpoint(const std::string& url);
    static bool tcp_probe(const std::string& host, const std::string& service,
                          int timeout_ms, std::string* error_message,
                          long long* elapsed_ms);
    bool is_duplicate_command(const json& payload_json);
    bool is_command_expired(const json& payload_json, long long local_now_ms, long long* age_ms) const;
    void update_state(GatewayState new_state, const char* reason);
    std::string state_name(GatewayState state) const;

    bool start_server(uint16_t port);
    bool start_client(const std::string &URL);
};
