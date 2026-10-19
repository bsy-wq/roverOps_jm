#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

class LocalWebsocketServer {
public:
    enum class Service { Equipment, Test };
    using Handle = websocketpp::connection_hdl;
    using MessageHandler = std::function<void(Handle, const std::string&, bool)>;

    LocalWebsocketServer(std::string ip, int port, std::string equipment_path,
                         std::string test_path);
    ~LocalWebsocketServer();
    bool start();
    void stop();
    void setMessageHandler(Service service, MessageHandler handler);
    void send(Handle connection, const std::string& payload, bool binary = false);
    void broadcast(Service service, const std::string& payload, bool binary = false);

private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    Server server_;
    std::string ip_, equipment_path_, test_path_;
    int port_;
    std::thread thread_;
    std::map<Handle, Service, std::owner_less<Handle>> connections_;
    std::mutex handler_mutex_;
    MessageHandler ai_handler_, light_handler_;
    bool started_ = false;
    bool used_ = false;
};
