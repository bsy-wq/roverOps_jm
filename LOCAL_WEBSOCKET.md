# Local WebSocket services

The robot_dog process starts both services using the robot.yaml file in its
working directory. They share one listener; they are separate URL routes,
not separate processes.

Current build/robot.yaml endpoints:

- AI: ws://192.168.44.102:9001/robot/ai/recognition
- Light: ws://192.168.44.102:9001/robot/light

The listener address, port and paths come from websocket.ip, websocket.port,
websocket.server_ai and websocket.server_light. Unknown paths are rejected
with HTTP 404. Multiple connections per service are supported. Message size
is limited to 1 MiB. WebSocket Ping/Pong is handled by websocketpp.

## Business integration

No business protocol, acknowledgment, automatic echo, or robot command is
implemented yet. Messages without a registered handler are ignored.
Register each handler before start() in src/main.cc:

```cpp
local_ws.setMessageHandler(LocalWebsocketServer::Service::Ai,
    [&local_ws](LocalWebsocketServer::Handle connection,
                const std::string& payload, bool binary) {
        // Add AI processing here when the protocol is defined.
        // local_ws.send(connection, response, binary);
    });
local_ws.setMessageHandler(LocalWebsocketServer::Service::Light,
    [&local_ws](LocalWebsocketServer::Handle connection,
                const std::string& payload, bool binary) {
        // Add light processing here when the protocol is defined.
    });
```

send() replies to one connection. broadcast(Service::Ai, payload) and
broadcast(Service::Light, payload) send only to that service's connections.
Both enqueue work on the server thread and may be called by other threads
while the service is running. Handlers run on the server thread: keep them
short and hand long operations to a worker. Call start()/stop() from the
owning thread, not from a message handler. A stopped instance cannot restart;
create a new instance when necessary. Destroy the instance after external
producers have stopped calling send()/broadcast().

## Verification

Run the standalone test without starting robot_dog or invoking robot actions:

```bash
g++ -std=c++17 -pthread -Isrc tests/local_websocket_smoke.cc \
  src/localWebsocket/LocalWebsocketServer.cc -lboost_system \
  -o /tmp/grebot_ws_smoke
python3 tests/local_websocket_smoke.py
```

The test uses 127.0.0.1:19091 and temporary echo callbacks to verify routing,
404 rejection, text/binary messages, Ping/Pong, broadcast isolation and shutdown.
Production handlers remain unset.
