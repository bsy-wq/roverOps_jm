# 运控 Web JSON 协议

本文档整理 `grebotMap_jm` 当前实现的 WebSocket 运控消息格式。

## 1. 通用格式

WebSocket 发送 JSON 文本，推荐使用 `action_control`（兼容 `control`）。当前机器人 ID 为 `M20-P1-bsy`，`timestamp` 使用 Unix 毫秒时间戳。

```json
{
  "type": "action_control",
  "device_id": "M20-P1-bsy",
  "timestamp": 1760097600000,
  "data": {
    "command": "move_forward",
    "parameters": {
      "linear_velocity": 0.3,
      "angular_velocity": 0.0
    }
  }
}
```

`data` 也支持直接放在顶层；`device_id` 也支持兼容字段 `robot_id`。

## 2. 动作示例

### 前进
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"move_forward","parameters":{"linear_velocity":0.3,"angular_velocity":0.0}}}
```

### 后退
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"move_backward","parameters":{"linear_velocity":-0.3,"angular_velocity":0.0}}}
```

### 左转
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"turn_left","parameters":{"linear_velocity":0.0,"angular_velocity":0.3}}}
```

### 右转
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"turn_right","parameters":{"linear_velocity":0.0,"angular_velocity":-0.3}}}
```

### 停止
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"stop"}}
```

### 站立
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"stand"}}
```

### 趴倒
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"down"}}
```

### 急停
```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"command":"emergency_stop"}}
```

当前实现的 `emergency_stop` 会将速度置零，但未接入独立硬件 `/estop` 接口；硬件级急停仍需使用实际急停设备。

## 3. 速度控制

可以不填写 `command`，直接发送速度：

```json
{"type":"action_control","device_id":"M20-P1-bsy","timestamp":1760097600000,"data":{"parameters":{"linear_velocity":0.3,"angular_velocity":0.0}}}
```

| 字段 | 类型 | 含义 |
|---|---|---|
| `linear_velocity` | number | 前后速度，单位 m/s；正数前进，负数后退 |
| `angular_velocity` | number | 旋转速度，单位 rad/s；正数左转，负数右转 |

当前实现只使用这两个速度字段，不使用横向速度字段；输入必须是有限数值。

## 4. 注意事项

1. 速度控制建议持续发送；约 2 秒没有新速度消息时，程序会停止持续运动并恢复空闲站立状态。
2. 需要停止时应明确发送 `stop`，不要只依赖超时。
3. 同一个 `stand` 或 `down` 命令在 3 秒内重复发送会被去重。
4. `device_id`/`robot_id` 存在且不是 `M20-P1-bsy` 时，消息会被忽略。
5. 顶层必须是合法 JSON 对象。

## 5. 程序内部 ROS 消息

解析后程序发布到 `/motion_cmd` 的字符串 JSON：

```json
{"source":"navigation","target_mode":"running","ttl_sec":1.0,"twist":{"x":0.3,"y":0.0,"z":0.0}}
```

站立和趴倒时 `target_mode` 分别为 `stand` 和 `lie_down`，速度均为 `0`。
