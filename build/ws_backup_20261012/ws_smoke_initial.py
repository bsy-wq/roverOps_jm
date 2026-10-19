import base64
import hashlib
import os
import socket
import struct
import subprocess

def connect(path, expected=101):
    sock = socket.create_connection(('127.0.0.1', 19091), timeout=3)
    key = base64.b64encode(os.urandom(16)).decode()
    sock.sendall(('GET ' + path + ' HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ' + key + '\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
    response = b''
    while b'\r\n\r\n' not in response:
        response += sock.recv(4096)
    assert int(response.split()[1]) == expected, response
    if expected == 101:
        accept = base64.b64encode(hashlib.sha1((key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest())
        assert accept in response
    return sock

def send(sock, data, opcode=1):
    mask = os.urandom(4)
    assert len(data) < 126
    sock.sendall(bytes([128 | opcode, 128 | len(data)]) + mask + bytes(value ^ mask[i % 4] for i, value in enumerate(data)))

def exact(sock, count):
    result = b''
    while len(result) < count:
        data = sock.recv(count - len(result))
        assert data, 'unexpected EOF'
        result += data
    return result

def receive(sock):
    opcode, size = exact(sock, 2)
    size &= 127
    if size == 126:
        size = struct.unpack('!H', exact(sock, 2))[0]
    return opcode & 15, exact(sock, size)

process = subprocess.Popen(['/tmp/grebot_ws_smoke'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
sockets = []
try:
    while process.stdout.readline().strip() != 'READY':
        assert process.poll() is None, 'server failed'
    ai = connect('/robot/ai/recognition')
    ai2 = connect('/robot/ai/recognition')
    light = connect('/robot/light')
    sockets.extend([ai, ai2, light])
    connect('/unknown', 404).close()
    send(ai, b'hello')
    assert receive(ai) == (1, b'ai:hello')
    send(light, b'\x00\x01', 2)
    assert receive(light) == (2, b'light:\x00\x01')
    send(ai, b'ping', 9)
    assert receive(ai) == (10, b'ping')
    send(ai, b'broadcast')
    assert receive(ai) == (1, b'ai-only')
    assert receive(ai2) == (1, b'ai-only')
    light.settimeout(0.3)
    try:
        receive(light)
        raise AssertionError('AI broadcast reached light service')
    except socket.timeout:
        pass
    light.settimeout(3)
    process.stdin.write('stop\n')
    process.stdin.flush()
    for sock in sockets:
        opcode, data = receive(sock)
        assert opcode == 8
        send(sock, data, 8)
        sock.close()
    assert process.wait(timeout=8) == 0
    print('PASS: routes, rejection, text, binary, ping/pong, broadcast isolation, shutdown')
finally:
    for sock in sockets:
        sock.close()
    if process.poll() is None:
        process.kill()
        process.wait()
