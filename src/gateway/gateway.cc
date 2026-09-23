#include "gateway.h"


gateway::gateway()
    : private_nh_("~")
{
    update_state(GatewayState::Disconnected, "constructor");
    private_nh_.param<int>("local_port", port, 9001);
    private_nh_.param<int>("ping_second", pingSecond, 2);
    private_nh_.param<int>("pong_timeout", pongTimeOut, 5);
    private_nh_.param<int>("command_timeout_ms", command_timeout_ms_, 60000);

    // 重连退避参数
    private_nh_.param<int>("reconnect_max_attempts", reconnect_max_attempts_, 0);
    private_nh_.param<int>("reconnect_base_delay_ms", reconnect_base_delay_ms_, 1000);
    private_nh_.param<int>("reconnect_max_delay_ms", reconnect_max_delay_ms_, 60000);

    ws_server.init_asio();
    ws_client.init_asio();

    server_strand = std::make_shared<strand_t>(ws_server.get_io_service().get_executor());
    client_strand = std::make_shared<strand_t>(ws_client.get_io_service().get_executor());

    ws_server.clear_access_channels(websocketpp::log::alevel::all);
    ws_client.clear_access_channels(websocketpp::log::alevel::all);

    ws_server.clear_error_channels(websocketpp::log::elevel::all);
    ws_client.clear_error_channels(websocketpp::log::elevel::all);

    ws_server.set_open_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_server_open, this, std::placeholders::_1)));
    ws_server.set_close_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_server_close, this, std::placeholders::_1)));
    ws_server.set_message_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_message, this, std::placeholders::_1, std::placeholders::_2)));

    ws_client.set_open_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_open, this, std::placeholders::_1)));
    ws_client.set_close_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_close, this, std::placeholders::_1)));
    ws_client.set_message_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_message, this, std::placeholders::_1, std::placeholders::_2)));
    ws_client.set_fail_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_fail, this, std::placeholders::_1)));

    ws_client.set_pong_timeout(pongTimeOut * 1000);
    ws_client.set_pong_handler(std::bind(&gateway::on_pong, this, std::placeholders::_1, std::placeholders::_2));
    ws_client.set_pong_timeout_handler(std::bind(
        &gateway::on_pong_timeout,
        this,
        std::placeholders::_1,
        std::placeholders::_2));
}

gateway::~gateway()
{
    stopping_.store(true, std::memory_order_release);
    update_state(GatewayState::Stopping, "destructor");
    ROS_INFO("[Gateway] 正在关闭系统...");
    ws_server.stop_listening();

    ws_server.stop();
    ws_client.stop();

    client_work.reset();

    if (server_thread.joinable())
        server_thread.join();
    if (client_thread.joinable())
        client_thread.join();

    ROS_INFO("[Gateway] 系统已安全退出");
}

bool gateway::start_server(uint16_t port)
{
    try
    {
        update_state(GatewayState::Connecting, "start_server");
        ws_server.listen(boost::asio::ip::tcp::v4(), port);
        ws_server.start_accept();
        if (!server_thread.joinable())
        {
            server_thread = std::thread([this, port]()
                                        {
                ROS_INFO("[Server] 服务器线程启动，监听端口: %d", port);
                ws_server.run(); });
        }
        return true;
    }
    catch (const std::exception &e)
    {
        ROS_ERROR("[Server] 启动失败: %s", e.what());
        return false;
    }
}

bool gateway::start_client(const std::string &URL)
{
    this->webURL = URL;
    update_web_endpoint(URL);
    if (!client_thread.joinable())
    {
        client_work = std::make_unique<work_guard_t>(ws_client.get_io_service().get_executor());
        client_thread = std::thread([this]()
                                    {
                                        client_thread_id = std::this_thread::get_id();
                                        ws_client.run();
                                    });
    }

    boost::asio::post(*client_strand, [this]()
                      { this->connect_to_web(); this->connect_heartbeat(); });
    return true;
}

void gateway::connect_heartbeat()
{
    if (stopping_.load(std::memory_order_acquire) || heartbeat_connected.load(std::memory_order_acquire)) return;
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(heartbeatURL, ec);
    if (ec) { ROS_ERROR("[Heartbeat] 获取连接失败: %s", ec.message().c_str()); return; }
    hdl_heartbeat = con->get_handle();
    ws_client.connect(con);
}

bool gateway::setup(const std::string &web_url)
{
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (!message_callback_)
        {
            ROS_ERROR("[Gateway] 设置失败: MessageCallback 尚未初始化！");
            return false;
        }
    }

    update_state(GatewayState::Connecting, "setup");
    webURL = web_url;
    update_web_endpoint(web_url);
    const auto scheme_end = web_url.find("://");
    const auto path_start = web_url.find('/', scheme_end == std::string::npos ? 0 : scheme_end + 3);
    heartbeatURL = web_url.substr(0, path_start) + "/robot/ws/robotDog/" + device_id_;

    // if (!this->start_server(port))
    // {
    //     ROS_ERROR("[Gateway] 无法启动本地服务器，端口: %d", port);
    //     return false;
    // }

    if (!this->start_client(webURL))
    {
        ROS_ERROR("[Gateway] 无法初始化远程连接: %s", webURL.c_str());
        return false;
    }

    return true;
}

void gateway::schedule_reconnect()
{
    if (stopping_.load(std::memory_order_acquire)) {
        return;
    }
    if (webserver_connected.load(std::memory_order_acquire) ||
        client_connecting_.load(std::memory_order_acquire)) {
        return;
    }
    bool expected = false;
    if (!reconnect_scheduled_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    if (reconnect_max_attempts_ > 0 && reconnect_attempts_ >= reconnect_max_attempts_) {
        reconnect_scheduled_.store(false, std::memory_order_release);
        ROS_ERROR("[Client] 已达到最大重连次数: %d", reconnect_max_attempts_);
        update_state(GatewayState::Degraded, "max reconnect attempts reached");
        return;
    }

    int delay_ms = reconnect_base_delay_ms_;
    if (reconnect_attempts_ == 0) {
        delay_ms = reconnect_base_delay_ms_;
    } else {
        delay_ms = reconnect_attempts_ * 10000;
    }
    delay_ms = std::min(delay_ms, reconnect_max_delay_ms_);

    reconnect_attempts_++;
    update_state(GatewayState::Reconnecting, "schedule reconnect");

    ROS_WARN("[Client] %.1f 秒后重连 (第 %d 次)", delay_ms / 1000.0, reconnect_attempts_);

    ws_client.set_timer(delay_ms, boost::asio::bind_executor(*client_strand,
            [this](websocketpp::lib::error_code const &ec)
            {
                reconnect_scheduled_.store(false, std::memory_order_release);
                if (ec) return;

                if (!stopping_.load(std::memory_order_acquire) &&
                    !webserver_connected.load(std::memory_order_acquire) &&
                    !client_connecting_.load(std::memory_order_acquire))
                {
                    this->connect_to_web();
                }
            }));
}

void gateway::connect_to_web()
{
    if (stopping_.load(std::memory_order_acquire)) {
        return;
    }

    if (webserver_connected.load(std::memory_order_acquire)) {
        return;
    }

    bool expected = false;
    if (!client_connecting_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        ROS_WARN("[Client] 已有连接正在建立，跳过重复 connect");
        return;
    }

    if (close_web_connection("connect_to_web cleanup")) {
        client_connecting_.store(false, std::memory_order_release);
        schedule_reconnect();
        return;
    }

    update_state(GatewayState::Connecting, "connect_to_web");
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(webURL, ec);
    if (!ec)
    {
        hdl_webserver = con->get_handle();
        ws_client.connect(con);
    }
    else
    {
        client_connecting_.store(false, std::memory_order_release);
        ROS_ERROR("[Client] 获取连接失败: %s，准备下一次重试...", ec.message().c_str());
        this->schedule_reconnect();
    }
}

bool gateway::is_same_hdl(connection_hdl lhs, connection_hdl rhs) const
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

void gateway::update_web_endpoint(const std::string& url)
{
    web_host_.clear();
    web_port_.clear();

    const auto scheme_pos = url.find("://");
    const auto authority_start = scheme_pos == std::string::npos ? 0 : scheme_pos + 3;
    const auto path_start = url.find('/', authority_start);
    std::string authority = url.substr(
        authority_start,
        path_start == std::string::npos ? std::string::npos : path_start - authority_start);

    const auto at_pos = authority.rfind('@');
    if (at_pos != std::string::npos) {
        authority = authority.substr(at_pos + 1);
    }

    if (!authority.empty() && authority.front() == '[') {
        const auto close = authority.find(']');
        if (close != std::string::npos) {
            web_host_ = authority.substr(1, close - 1);
            if (close + 1 < authority.size() && authority[close + 1] == ':') {
                web_port_ = authority.substr(close + 2);
            }
        }
    } else {
        const auto colon = authority.rfind(':');
        if (colon != std::string::npos) {
            web_host_ = authority.substr(0, colon);
            web_port_ = authority.substr(colon + 1);
        } else {
            web_host_ = authority;
        }
    }

    if (web_port_.empty()) {
        web_port_ = url.rfind("wss://", 0) == 0 ? "443" : "80";
    }
}

bool gateway::tcp_probe(const std::string& host, const std::string& service,
                               int timeout_ms, std::string* error_message,
                               long long* elapsed_ms)
{
    const auto started = std::chrono::steady_clock::now();
    boost::asio::io_context io;
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::ip::tcp::socket socket(io);
    boost::asio::steady_timer timer(io);
    boost::system::error_code result_ec = boost::asio::error::would_block;
    std::string stage = "resolve";
    bool completed = false;

    timer.expires_after(std::chrono::milliseconds(timeout_ms));
    timer.async_wait([&](const boost::system::error_code& ec) {
        if (!ec && !completed) {
            result_ec = boost::asio::error::timed_out;
            stage = "timeout";
            boost::system::error_code ignored;
            socket.close(ignored);
            resolver.cancel();
        }
    });

    resolver.async_resolve(host, service,
        [&](const boost::system::error_code& ec,
            boost::asio::ip::tcp::resolver::results_type endpoints) {
            if (ec) {
                completed = true;
                result_ec = ec;
                stage = "resolve";
                timer.cancel();
                return;
            }

            stage = "connect";
            boost::asio::async_connect(socket, endpoints,
                [&](const boost::system::error_code& connect_ec,
                    const boost::asio::ip::tcp::endpoint&) {
                    completed = true;
                    result_ec = connect_ec;
                    timer.cancel();
                });
        });

    io.run();

    if (elapsed_ms) {
        *elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
    }
    if (error_message) {
        if (!result_ec) {
            *error_message = "ok";
        } else {
            *error_message = stage + ": " + result_ec.message();
        }
    }
    return !result_ec;
}

void gateway::run_network_diagnostics(const char* reason)
{
    bool expected = false;
    if (!network_diagnostic_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        ROS_WARN("[NetDiag] 已有网络诊断正在运行，跳过本次: reason=%s", reason);
        return;
    }

    const std::string reason_copy = reason ? reason : "unknown";
    const std::string target_host = web_host_;
    const std::string target_port = web_port_;
    const std::string url = webURL;

    std::thread([this, reason_copy, target_host, target_port, url]() {
        ROS_WARN("[NetDiag] WebSocket 断连诊断开始: reason=%s, url=%s, target=%s:%s",
                 reason_copy.c_str(), url.c_str(), target_host.c_str(), target_port.c_str());

        std::string target_error;
        long long target_ms = 0;
        bool target_ok = false;
        if (!target_host.empty() && !target_port.empty()) {
            target_ok = tcp_probe(target_host, target_port, 3000, &target_error, &target_ms);
        } else {
            target_error = "target endpoint parse failed";
        }

        std::string external_error;
        long long external_ms = 0;
        const bool external_ok = tcp_probe("223.5.5.5", "53", 3000, &external_error, &external_ms); // 阿里公共 DNS ip 和 port

        ROS_WARN("[NetDiag] 目标服务 TCP 探测: %s:%s %s, cost=%lld ms, detail=%s",
                 target_host.c_str(), target_port.c_str(),
                 target_ok ? "OK" : "FAILED",
                 target_ms, target_error.c_str());
        ROS_WARN("[NetDiag] 外网 TCP 探测: 223.5.5.5:53 %s, cost=%lld ms, detail=%s",
                 external_ok ? "OK" : "FAILED",
                 external_ms, external_error.c_str());

        if (!external_ok) {
            ROS_ERROR("[NetDiag] 判断: 本机/局域网出口可能异常，外网探测失败");
        } else if (!target_ok) {
            ROS_ERROR("[NetDiag] 判断: 本机外网正常，但目标服务 TCP 不通，偏向对方服务或中间链路问题");
        } else {
            ROS_ERROR("[NetDiag] 判断: 本机外网正常，目标 TCP 也通；WebSocket 断开更偏向服务端协议/会话状态/中间代理关闭");
        }

        network_diagnostic_running_.store(false, std::memory_order_release);
    }).detach();
}

bool gateway::close_web_connection(const char* reason)
{
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_con_from_hdl(hdl_webserver, ec);
    if (ec || !con) {
        return false;
    }

    const auto state = con->get_state();
    if (state == websocketpp::session::state::closed) {
        return false;
    }

    ROS_WARN("[Client] 关闭旧 WebSocket 连接: %s", reason);
    if (state == websocketpp::session::state::open) {
        ws_client.close(hdl_webserver, websocketpp::close::status::going_away, reason, ec);
        if (!ec) {
            return true;
        }
        ROS_WARN("[Client] 正常关闭旧连接失败: %s，强制终止", ec.message().c_str());
    }

    con->terminate(websocketpp::lib::error_code());
    return true;
}

void gateway::mark_web_disconnected(const char* reason, bool reconnect)
{
    webserver_connected.store(false, std::memory_order_release);
    client_connecting_.store(false, std::memory_order_release);
    update_state(GatewayState::Degraded, reason);
    connection_generation_.fetch_add(1, std::memory_order_acq_rel);
    run_network_diagnostics(reason);

    auto cleanup_and_reconnect = [this, reason, reconnect]() {
        close_web_connection(reason);
        if (reconnect) {
            schedule_reconnect();
        }
    };

    if (std::this_thread::get_id() == client_thread_id) {
        cleanup_and_reconnect();
    } else {
        boost::asio::post(*client_strand, cleanup_and_reconnect);
    }
}

bool gateway::send_to_web_now(const std::string& payload)
{
    const bool is_task_round_completed =
        payload.find("\"event\":\"task_round_completed\"") != std::string::npos;
    const bool is_nav_bridge_robot_status =
        payload.find("\"type\":\"robot_status\"") != std::string::npos;

    if (!webserver_connected.load(std::memory_order_acquire)) {
        if (is_task_round_completed) {
            LOG_WARN("[Client] task_round_completed 未发送: websocket 未连接, payload={}",
                     payload);
        }
        if (is_nav_bridge_robot_status) {
            LOG_WARN("[Client] nav_bridge robot_status 未发送: websocket 未连接, payload={}",
                     payload);
        }
        return false;
    }

    websocketpp::lib::error_code ec;
    auto con = ws_client.get_con_from_hdl(hdl_webserver, ec);
    if (ec || !con) {
        ROS_ERROR("[Client] 获取连接失败: %s", ec.message().c_str());
        if (is_task_round_completed) {
            LOG_ERROR("[Client] task_round_completed 获取连接失败: {}, payload={}",
                      ec.message(), payload);
        }
        if (is_nav_bridge_robot_status) {
            LOG_ERROR("[Client] nav_bridge robot_status 获取连接失败: {}, payload={}",
                      ec.message(), payload);
        }
        mark_web_disconnected("send get_con failed", true);
        return false;
    }

    if (con->get_state() != websocketpp::session::state::open) {
        ROS_WARN("[Client] websocket 未 OPEN");
        if (is_task_round_completed) {
            LOG_WARN("[Client] task_round_completed 未发送: websocket 状态不是 OPEN, payload={}",
                     payload);
        }
        if (is_nav_bridge_robot_status) {
            LOG_WARN("[Client] nav_bridge robot_status 未发送: websocket 状态不是 OPEN, payload={}",
                     payload);
        }
        mark_web_disconnected("send not open", true);
        return false;
    }

    if (is_task_round_completed) {
        LOG_INFO("[Client] task_round_completed 开始调用 websocket send, payload={}",
                 payload);
    }
    ws_client.send(hdl_webserver, payload, websocketpp::frame::opcode::text, ec);
    if (ec) {
        ROS_ERROR("[Client] 发送失败: %s", ec.message().c_str());
        if (is_task_round_completed) {
            LOG_ERROR("[Client] task_round_completed websocket send 失败: {}, payload={}",
                      ec.message(), payload);
        }
        if (is_nav_bridge_robot_status) {
            LOG_ERROR("[Client] nav_bridge robot_status websocket send 失败: {}, payload={}",
                      ec.message(), payload);
        }
        mark_web_disconnected("send failed", true);
        return false;
    }
    if (is_task_round_completed) {
        LOG_INFO("[Client] task_round_completed websocket send 成功, bytes={}, payload={}",
                 payload.size(), payload);
    }
    if (is_nav_bridge_robot_status) {
        LOG_INFO("[Client] nav_bridge robot_status websocket send 成功, bytes={}, payload={}",
                 payload.size(), payload);
    }
    return true;
}

void gateway::schedule_heartbeat(int generation)
{
    if (pingSecond <= 0 || stopping_.load(std::memory_order_acquire)) {
        return;
    }

    ws_client.set_timer(pingSecond * 1000, boost::asio::bind_executor(*client_strand,
        [this, generation](websocketpp::lib::error_code const &ec)
        {
            if (stopping_.load(std::memory_order_acquire)) {
                return;
            }
            if (ec) {
                if (heartbeat_connected.load(std::memory_order_acquire)) {
                    ROS_WARN("[Client] heartbeat timer 异常: %s, 重新调度",
                             ec.message().c_str());
                    schedule_heartbeat(generation);
                }
                return;
            }
            if (!heartbeat_connected.load(std::memory_order_acquire)) {
                schedule_heartbeat(generation);
                return;
            }

            const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            const json heartbeat = {
                {"type", "nav_bridge_ping"},
                // {"type", "ping"},
                {"device_id", device_id_},
                {"timestamp", timestamp_ms}
            };

            if (!send_heartbeat_now(heartbeat.dump())) {
                ROS_ERROR("[Client] heartbeat message 发送失败");
                return;
            }

            const auto ping_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            websocketpp::lib::error_code ping_ec;
            ws_client.ping(hdl_heartbeat, std::to_string(ping_timestamp_ms), ping_ec);
            if (ping_ec) {
                ROS_ERROR("[Client] WebSocket Ping 发送失败: %s", ping_ec.message().c_str());
                mark_web_disconnected("websocket ping failed", true);
                return;
            }

            schedule_heartbeat(generation);
        }));
}

void gateway::set_device_id(const std::string& device_id)
{
    device_id_ = device_id;
}

bool gateway::send_heartbeat_now(const std::string& payload)
{
    if (!heartbeat_connected.load(std::memory_order_acquire)) {
        return false;
    }
    websocketpp::lib::error_code ec;
    ws_client.send(hdl_heartbeat, payload, websocketpp::frame::opcode::text, ec);
    if (ec) {
        ROS_ERROR("[Heartbeat] nav_bridge_ping 发送失败: %s", ec.message().c_str());
        heartbeat_connected.store(false, std::memory_order_release);
        return false;
    }
    // ROS_INFO("[Heartbeat] nav_bridge_ping 已发送到 %s: %s", heartbeatURL.c_str(), payload.c_str());
    return true;
}

void gateway::on_server_open(connection_hdl hdl)
{
    hdl_handset = hdl;
    handset_connected.store(true, std::memory_order_release);
    ROS_INFO("[Server] 遥控器已连接");
}

void gateway::on_server_close(connection_hdl hdl)
{
    handset_connected.store(false, std::memory_order_release);
    ROS_INFO("[Server] 遥控器连接断开");
}

void gateway::on_client_open(connection_hdl hdl)
{
    if (is_same_hdl(hdl, hdl_heartbeat)) {
        heartbeat_connected.store(true, std::memory_order_release);
        ROS_INFO("[Heartbeat] 已连接: %s", heartbeatURL.c_str());
        schedule_heartbeat(0);
        return;
    }
    if (!is_same_hdl(hdl, hdl_webserver)) {
        websocketpp::lib::error_code close_ec;
        ws_client.close(hdl, websocketpp::close::status::going_away, "stale connection", close_ec);
        ROS_WARN("[Client] 忽略并关闭过期 WebSocket 连接");
        return;
    }

    hdl_webserver = hdl;
    client_connecting_.store(false, std::memory_order_release);
    webserver_connected.store(true, std::memory_order_release);
    reconnect_scheduled_.store(false, std::memory_order_release);
    reconnect_attempts_ = 0;  // 重置重试计数器
    const int generation = connection_generation_.fetch_add(1, std::memory_order_acq_rel) + 1;
    last_web_packet_time = std::chrono::steady_clock::now();
    last_web_pong_time = last_web_packet_time;
    ws_client.set_timer(30000, boost::asio::bind_executor(*client_strand,
            [this, generation](websocketpp::lib::error_code const &timer_ec)
            {
                if (!timer_ec &&
                    webserver_connected.load(std::memory_order_acquire) &&
                    connection_generation_.load(std::memory_order_acquire) == generation)
                {
                    kicked_reconnect_attempts_ = 0;
                }
            }));

    // TODO: 可以关闭 Nagle 算法，指令立即接受和发出
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_con_from_hdl(hdl, ec);
    if (ec || !con) {
        ROS_ERROR("[Client] 获取连接句柄失败: %s", ec.message().c_str());
        webserver_connected.store(false, std::memory_order_release);
        update_state(GatewayState::Degraded, "client open get_con failed");
        schedule_reconnect();
        return;
    }
    con->get_socket().set_option(boost::asio::ip::tcp::no_delay(true));
    con->get_socket().set_option(boost::asio::ip::tcp::socket::keep_alive(true));

    update_state(GatewayState::Connected, "client open");
    ROS_INFO("[Client] 已连接到 Web 服务器");
    schedule_heartbeat(generation);

    std::function<void()> connected_cb;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        connected_cb = connected_callback_;
    }
    if (connected_cb) {
        try {
            connected_cb();
        } catch (const std::exception& e) {
            ROS_ERROR("[Client] connected callback 异常: %s", e.what());
        }
    }
}

void gateway::on_client_close(connection_hdl hdl)
{
    if (is_same_hdl(hdl, hdl_heartbeat)) {
        heartbeat_connected.store(false, std::memory_order_release);
        ROS_WARN("[Heartbeat] robotDog WebSocket 已断开");
        if (!stopping_.load(std::memory_order_acquire)) {
            ws_client.set_timer(2000, boost::asio::bind_executor(*client_strand,
                [this](websocketpp::lib::error_code const &ec) { if (!ec) connect_heartbeat(); }));
        }
        return;
    }
    if (!is_same_hdl(hdl, hdl_webserver)) {
        ROS_WARN("[Client] 忽略过期 WebSocket close 事件");
        return;
    }

    const bool kicked_by_new_connection = kicked_by_new_default_connection_.exchange(false, std::memory_order_acq_rel);
    client_connecting_.store(false, std::memory_order_release);
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_con_from_hdl(hdl, ec);
    if (!ec && con) {
        ROS_WARN("[Client] Web 服务器断开: code=%d, reason=%s",
                 con->get_remote_close_code(),
                 con->get_remote_close_reason().c_str());
    } else {
        ROS_WARN("[Client] Web 服务器断开，无法读取关闭原因: %s", ec.message().c_str());
    }
    if (kicked_by_new_connection) {
        kicked_reconnect_attempts_ = std::min(kicked_reconnect_attempts_ + 1, 6);
        reconnect_attempts_ = std::max(reconnect_attempts_, kicked_reconnect_attempts_);
        mark_web_disconnected("client close after kicked by new connection", true);
    } else {
        kicked_reconnect_attempts_ = 0;
        mark_web_disconnected("client close", true);
    }
}

void gateway::on_client_fail(connection_hdl hdl)
{
    if (!is_same_hdl(hdl, hdl_webserver)) {
        ROS_WARN("[Client] 忽略过期 WebSocket fail 事件");
        return;
    }

    client_connecting_.store(false, std::memory_order_release);
    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_con_from_hdl(hdl, ec);
    if (!ec && con) {
        websocketpp::lib::error_code transport_ec = con->get_ec();
        ROS_ERROR("[Client] 连接 Web 服务器失败: %s", transport_ec.message().c_str());
    } else {
        ROS_ERROR("[Client] 连接 Web 服务器失败，无法读取失败原因: %s", ec.message().c_str());
    }
    mark_web_disconnected("client fail", true);
}

void gateway::on_pong(connection_hdl hdl, std::string payload)
{
    last_web_pong_time = std::chrono::steady_clock::now();
    try
    {
        auto now = std::chrono::steady_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        long long sent_ms = std::stoll(payload);
        auto latency = now_ms - sent_ms;

        if (latency > 1000)
        {
            ROS_WARN("[Client] 网络延迟较高: %lld ms", latency);
        }
    }
    catch (...)
    {
        ROS_INFO("[Client] 收到标准 Pong 响应");
    }
}

void gateway::on_pong_timeout(connection_hdl hdl, std::string payload)
{
    if (!is_same_hdl(hdl, hdl_webserver) ||
        !webserver_connected.load(std::memory_order_acquire)) {
        return;
    }

    ROS_ERROR("[Client] WebSocket Pong 超时 %d ms，主动断开并重连, payload=%s",
              pongTimeOut * 1000,
              payload.c_str());
    mark_web_disconnected("websocket pong timeout", true);
}

bool gateway::is_duplicate_command(const json& payload_json)
{
    std::string id;
    for (const char* key : {"command_id", "cmd_id", "request_id", "msg_id"}) {
        if (payload_json.contains(key)) {
            if (payload_json[key].is_string()) {
                id = payload_json[key].get<std::string>();
            } else if (payload_json[key].is_number_integer()) {
                id = std::to_string(payload_json[key].get<long long>());
            }
            if (!id.empty()) {
                id = payload_json.value("type", "") + ":" + id;
                break;
            }
        }
    }

    if (id.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(dedup_mutex_);
    if (recent_command_id_set_.find(id) != recent_command_id_set_.end()) {
        ROS_WARN("[Gateway] 重复命令已忽略: %s", id.c_str());
        return true;
    }

    recent_command_ids_.push_back(id);
    recent_command_id_set_.insert(id);
    while (recent_command_ids_.size() > kRecentCommandCacheLimit_) {
        recent_command_id_set_.erase(recent_command_ids_.front());
        recent_command_ids_.pop_front();
    }
    return false;
}

bool gateway::is_command_expired(const json& payload_json, long long local_now_ms, long long* age_ms) const
{
    if (command_timeout_ms_ <= 0 || !payload_json.contains("timestamp") ||
        !payload_json["timestamp"].is_number()) {
        return false;
    }

    long long remote_send_time_ms = payload_json["timestamp"].get<long long>();
    if (remote_send_time_ms > 0 && remote_send_time_ms < 100000000000LL) {
        remote_send_time_ms *= 1000;
    }

    const long long age = local_now_ms - remote_send_time_ms;
    if (age_ms) {
        *age_ms = age;
    }
    return age > command_timeout_ms_;
}

void gateway::broadcast_data(const std::string &payload)
{
    boost::asio::post(*server_strand, [this, payload]()
                      {
        if (handset_connected.load(std::memory_order_acquire))
        {
            websocketpp::lib::error_code ec;
            ws_server.send(hdl_handset, payload, websocketpp::frame::opcode::text, ec);
            if (ec)
            {
                ROS_ERROR("[Server] 发送到遥控器失败: %s", ec.message().c_str());
                handset_connected.store(false, std::memory_order_release);
                update_state(GatewayState::Degraded, "broadcast server send failed");
            }
        } });

    boost::asio::post(*client_strand, [this, payload]()
    {
        send_to_web_now(payload);
    });
}

void gateway::on_message(connection_hdl hdl, server::message_ptr msg)
{
    auto t1 = std::chrono::steady_clock::now();

    std::string payload = msg->get_payload();

    if (payload.empty()) {
        return;
    }

    // 过滤非 JSON 数据
    char c = payload[0];

    bool maybe_json =
        c == '{' ||
        c == '[' ||
        c == '"' ||
        c == '-' ||
        (c >= '0' && c <= '9') ||
        c == 't' ||
        c == 'f' ||
        c == 'n';

    if (!maybe_json) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    this->last_web_packet_time = std::chrono::steady_clock::now();

    json payload_json;

    try {

        payload_json = json::parse(payload);

    } catch (const std::exception& e) {

        ROS_ERROR_THROTTLE(
            2.0,
            "[Gateway] JSON parse failed: %s",
            e.what()
        );

        return;
    }

    const std::string payload_type = payload_json.value("type", "");
    // if (payload_type == "nav_bridge_ping" || payload_type == "heartbeat") {
    //     return;
    // }

    if (payload_type == "kicked" &&
        payload_json.value("reason", "") == "new_connection") {
        kicked_by_new_default_connection_.store(true, std::memory_order_release);
        ROS_WARN("[Client] 当前 default 连接被新连接替换，进入延迟重连保护");
        return;
    }

    if (payload_json.contains("timestamp") &&
        payload_json["timestamp"].is_number())
    {
        long long local_now_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count();

        long long remote_send_time_ms =
            payload_json["timestamp"].get<long long>();

        long long network_delay =
            local_now_ms - remote_send_time_ms;

        // ROS_INFO(
        //     "[Gateway] 收到消息类型: %s, 物理延迟: %lld ms",
        //     payload_json.value("type", "unknown").c_str(),
        //     network_delay
        // );
    }

    long long command_age_ms = 0;
    if (is_command_expired(payload_json, std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count(), &command_age_ms)) {
        ROS_WARN("[Gateway] 命令超时已忽略: type=%s, age=%lld ms, timeout=%d ms",
                 payload_json.value("type", "unknown").c_str(),
                 command_age_ms,
                 command_timeout_ms_);
        return;
    }

    if (is_duplicate_command(payload_json)) {
        return;
    }

    MessageCallback cb;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        cb = message_callback_;
    }
    if (cb) {
        auto invoke_callback = [cb, payload_json]() {
            try {
                cb(payload_json);
            } catch (const std::exception& e) {
                ROS_ERROR("[Gateway] message callback 异常: %s", e.what());
            } catch (...) {
                ROS_ERROR("[Gateway] message callback 未知异常");
            }
        };

        if (payload_type == "control" || payload_type == "action_control") {
            invoke_callback();
        } else {
            std::thread(invoke_callback).detach();
        }
    }

    if (!hdl.owner_before(hdl_handset) &&
        !hdl_handset.owner_before(hdl))
    {
        std::string dump_str = payload_json.dump();

        boost::asio::post(
            *client_strand,
            [this, dump_str]()
        {
            if (!webserver_connected.load(
                    std::memory_order_acquire))
            {
                return;
            }

            send_to_web_now(dump_str);
        });
    }

    auto t2 = std::chrono::steady_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            t2 - t1
        ).count();

    if (duration > 10)
    {
        ROS_INFO(
            "[Check] on_message 内部执行异常耗时: %ld ms",
            duration
        );
    }
}

void gateway::setMessageCallback(MessageCallback cb)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    message_callback_ = std::move(cb);
}

void gateway::setConnectedCallback(std::function<void()> cb)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    connected_callback_ = std::move(cb);
}

bool gateway::isWebConnected() const
{
    return webserver_connected.load(std::memory_order_acquire);
}

bool gateway::sendToWeb(const std::string& payload)
{
    if (!webserver_connected.load(std::memory_order_acquire))
    {
        return false;
    }

    try
    {
        if (std::this_thread::get_id() == client_thread_id) {
            return send_to_web_now(payload);
        }

        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        boost::asio::post(*client_strand, [this, payload, promise]()
        {
            try {
                promise->set_value(send_to_web_now(payload));
            } catch (const std::exception& e) {
                ROS_ERROR("[Client] sendToWeb 异常: %s", e.what());
                mark_web_disconnected("sendToWeb exception", true);
                promise->set_value(false);
            }
        });

        if (future.wait_for(std::chrono::milliseconds(2000)) != std::future_status::ready) {
            ROS_ERROR("[Client] sendToWeb 等待超时");
            mark_web_disconnected("sendToWeb wait timeout", true);
            return false;
        }
        return future.get();
    }
    catch (const std::exception& e)
    {
        ROS_ERROR("[Client] post sendToWeb 失败: %s", e.what());
        return false;
    }
}

std::string gateway::state_name(GatewayState state) const
{
    switch (state) {
        case GatewayState::Disconnected: return "Disconnected";
        case GatewayState::Connecting: return "Connecting";
        case GatewayState::Connected: return "Connected";
        case GatewayState::Reconnecting: return "Reconnecting";
        case GatewayState::Degraded: return "Degraded";
        case GatewayState::Stopping: return "Stopping";
    }
    return "Unknown";
}

void gateway::update_state(GatewayState new_state, const char* reason)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_ == new_state) {
        return;
    }

    ROS_INFO("[GatewayFSM] %s -> %s (%s)",
             state_name(state_).c_str(),
             state_name(new_state).c_str(),
             reason ? reason : "");
    LOG_INFO("[GatewayFSM] {} -> {} ({})",
             state_name(state_),
             state_name(new_state),
             reason ? reason : "");
    state_ = new_state;
}

std::string gateway::get_state_name() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_name(state_);
}
