#include "LocalWebsocketServer.h"
#include <iostream>
#include <utility>

LocalWebsocketServer::LocalWebsocketServer(std::string ip, int port,
                                         std::string equipment_path, std::string test_path)
    : ip_(std::move(ip)), equipment_path_(std::move(equipment_path)),
      test_path_(std::move(test_path)), port_(port) {
    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.init_asio();
    server_.set_reuse_addr(true);
    server_.set_max_message_size(1024 * 1024);
    server_.set_validate_handler([this](Handle hdl) {
        const auto connection = server_.get_con_from_hdl(hdl);
        const auto path = connection->get_uri()->get_resource();
        if (path == equipment_path_ || path == test_path_) return true;
        connection->set_status(websocketpp::http::status_code::not_found);
        connection->set_body("Unknown WebSocket service");
        return false;
    });
    server_.set_open_handler([this](Handle hdl) {
        const auto path = server_.get_con_from_hdl(hdl)->get_uri()->get_resource();
        connections_.emplace(hdl, path == equipment_path_ ? Service::Equipment : Service::Test);
        std::cout << "[Local WS] connected: " << path << std::endl; // localhost:9002/robot/ws/superstructure/equirpment/M20-P1-bsy
    });
    server_.set_close_handler([this](Handle hdl) { connections_.erase(hdl); });
    server_.set_message_handler([this](Handle hdl, Server::message_ptr message) {
        const auto it = connections_.find(hdl);
        if (it == connections_.end()) return;
        MessageHandler handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = it->second == Service::Equipment ? ai_handler_ : light_handler_;
        }
        // Business handlers are supplied separately for the two services.
        if (handler) {
            try {
                handler(hdl, message->get_payload(),
                        message->get_opcode() == websocketpp::frame::opcode::binary);
            } catch (const std::exception& e) {
                std::cerr << "[Local WS] message handler failed: " << e.what() << std::endl;
            }
        }
    });
}

LocalWebsocketServer::~LocalWebsocketServer() { stop(); }

bool LocalWebsocketServer::start() {
    if (started_) return true;
    if (used_) return false;
    if (port_ < 1 || port_ > 65535 || equipment_path_.empty() || test_path_.empty() ||
        equipment_path_.front() != '/' || test_path_.front() != '/' || equipment_path_ == test_path_) {
        std::cerr << "[Local WS] Invalid port or service paths" << std::endl;
        return false;
    }
    try {
        const auto address = boost::asio::ip::make_address(ip_);
        server_.listen(boost::asio::ip::tcp::endpoint(address, static_cast<unsigned short>(port_)));
        server_.start_accept();
        thread_ = std::thread([this] {
            try { server_.run(); }
            catch (const std::exception& e) {
                std::cerr << "[Local WS] event loop failed: " << e.what() << std::endl;
            }
        });
        used_ = started_ = true;
        std::cout << "[Local WS] listening on " << ip_ << ':' << port_
                  << " paths: " << equipment_path_ << ", " << test_path_ << std::endl;
        return true;
    } catch (const std::exception& e) {
        websocketpp::lib::error_code ec;
        server_.stop_listening(ec);
        std::cerr << "[Local WS] start failed: " << e.what() << std::endl;
        return false;
    }
}

void LocalWebsocketServer::stop() {
    if (!started_) return;
    server_.get_io_service().post([this] {
        websocketpp::lib::error_code ec;
        server_.stop_listening(ec);
        for (const auto& entry : connections_) {
            server_.close(entry.first, websocketpp::close::status::going_away,
                          "Server shutting down", ec);
        }
    });
    if (thread_.joinable()) thread_.join();
    connections_.clear();
    started_ = false;
}

void LocalWebsocketServer::setMessageHandler(Service service, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    (service == Service::Equipment ? ai_handler_ : light_handler_) = std::move(handler);
}

void LocalWebsocketServer::send(Handle connection, const std::string& payload, bool binary) {
    server_.get_io_service().post([this, connection, payload, binary] {
        if (connections_.find(connection) == connections_.end()) return;
        websocketpp::lib::error_code ec;
        server_.send(connection, payload, binary ? websocketpp::frame::opcode::binary
                                               : websocketpp::frame::opcode::text, ec);
        if (ec) std::cerr << "[Local WS] send failed: " << ec.message() << std::endl;
    });
}

void LocalWebsocketServer::broadcast(Service service, const std::string& payload, bool binary) {
    server_.get_io_service().post([this, service, payload, binary] {
        for (const auto& entry : connections_) {
            if (entry.second != service) continue;
            websocketpp::lib::error_code ec;
            server_.send(entry.first, payload, binary ? websocketpp::frame::opcode::binary
                                                     : websocketpp::frame::opcode::text, ec);
            if (ec) std::cerr << "[Local WS] broadcast failed: " << ec.message() << std::endl;
        }
    });
}
