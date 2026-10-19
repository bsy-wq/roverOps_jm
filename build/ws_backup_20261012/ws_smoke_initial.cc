#include "src/localWebsocket/LocalWebsocketServer.h"
#include <iostream>
int main() {
    LocalWebsocketServer server("127.0.0.1", 19091, "/robot/ai/recognition", "/robot/light");
    server.setMessageHandler(LocalWebsocketServer::Service::Ai,
        [&](auto hdl, const std::string& data, bool binary) {
            if (data == "broadcast") server.broadcast(LocalWebsocketServer::Service::Ai, "ai-only");
            else server.send(hdl, "ai:" + data, binary);
        });
    server.setMessageHandler(LocalWebsocketServer::Service::Light,
        [&](auto hdl, const std::string& data, bool binary) { server.send(hdl, "light:" + data, binary); });
    if (!server.start()) return 1;
    std::cout << "READY" << std::endl;
    std::string line;
    std::getline(std::cin, line);
    server.stop();
}
