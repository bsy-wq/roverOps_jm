# 全部业务与通信接口

范围：本项目所有已分发的业务命令、所有对外消息类别、直接使用的 ROS/HTTP/设备接口。下游 FAST LIO、PCT、SCAN 的内部算法接口不等于本项目接口；本文列出与网关流程相关的部分。参数完整白名单和源码字段索引见 [04_REFERENCE.md](04_REFERENCE.md)。

## 1. 连接与公共约定

### 1.1 通信端点

| 通道 | 当前配置计算结果 / 地址 | 方向及用途 |
| --- | --- | --- |
| Web 业务 WS | `ws://0.0.0.0:8080/default/M20-P1-bsy` | robot_dog 主动连接；下发命令、回包、位置、导航状态 |
| Web 心跳 WS | `ws://0.0.0.0:8080/robot/ws/robotDog/M20-P1-bsy` | 网关另建连接；JSON 心跳及 WS Ping/Pong |
| BMS WS | `ws://0.0.0.0:8080/robot/ws/robotDog/M20-P1-bsy` | BmsForwarder 独立连接；电池及另一种 robot_status |
| 本地设备 WS | `ws://192.168.44.102:9002/robot/ws/superstructure/equirpment/M20-P1-bsy` | 外设连板子；设备指令、灯控广播、AI ACK 回传 |
| 本地测试 WS | `ws://192.168.44.102:9002/robot/ws/bsy` | 能建立连接，main 没注册业务消息处理器 |
| 地图 HTTP | `http://112.2.6.99:8503` | 地图元数据、文件上传/下载、SN 注册 |
| 跟随 HTTP | `http://127.0.0.1:8088` | 由 follow_base_url ROS 参数覆盖 |
| 海康 HTTP | `http://192.168.44.64` | ISAPI 云台，默认通道 1，Digest 认证 |

`0.0.0.0` 是当前 YAML 的 gateway.host 原值，不能理解为外部服务器的真实 IP；部署时应按实际 Web 主机配置。`equirpment` 是源码/配置的实际拼写，不能改成 equipment。现有旧文档中的 9001、`/robot/ai/recognition`、`/robot/light` 不适用于当前 main。

本地 WS 多客户端广播，路由必须精确匹配，其他路径拒绝。单消息限制 1 MiB。main 只给 Equipment 注册 AI ACK 处理器，因此不能把导航/运控 JSON 直接发给 9002 当作业务入口。旧 gateway 自带的遥控器 WS 服务启动已被注释。

### 1.2 普通 Web 命令信封

```json
{"type":"get_fsm_state","device_id":"M20-P1-bsy","command_id":"unique-request-001"}
```

| 字段 | 规则 |
| --- | --- |
| type | 字符串，必需；具体命令见下表 |
| device_id / robot_id | 执行器优先检查 device_id，再检查 robot_id；存在时须匹配本机。普通命令可省略，设备透传命令必须有匹配 device_id |
| command_id / cmd_id / request_id / msg_id | 普通命令取首个可用字符串/整型 ID，连同 type 去重；缓存最近 100 项。不保证原样回传关联 ID |
| timestamp | 数值可选；小于 1e11 当秒转毫秒，否则按毫秒；默认超过本机 60 秒则丢弃，无统一拒绝回包。缺省不检查，未来时间当前不拒绝 |
| data | 仅部分命令支持，不能把全部命令一律包在 data 中 |

命令示例故意不填固定 timestamp，避免复制后因过期被丢弃。重复命令使用新的 command_id，避免去重。未知 type 没有通用 unsupported 回包。

大多数业务先回 `{type:原命令,status:"received",localTime:本地时间字符串}`，仅表示收到。FSM 不允许时回 `fsm_state.status=rejected`；受管命令忙时回 `{type:请求,status:"busy",code:409,active_command,localTime,message}`。不是每条命令都有统一的最终 ACK。

普通命令一般在后台线程执行；运控走直接回调。导航/建图使用受管命令锁，stop_navigation 可打断导航请求；运动、HTTP 跟随、参数等接口未统一纳入行为互斥。

### 1.3 时间戳和标识必须按消息区分

| 输出类别 | timestamp 单位与注意事项 |
| --- | --- |
| upload_position、upload_path、nav_bridge_ping、navbridge 的 robot_status | 毫秒 |
| ack_initial_pose、initial_pose_ready、ack_navigation_task、ack_clear_waypoints、建图 ACK、ack_set_dock_pose、recharge_status、navigation_planning_failed | 秒 |
| navigation_task_state、navigation_waypoint_reached、当前源码的灯控 | 秒。共用 `include/common/utils.h::unix_timestamp_ms()`，该函数实际返回 time_t 秒，名称误导 |
| BMS battery_status | BmsForwarder 自己的 unix_timestamp_ms，毫秒 |
| BMS robot_status | 秒，robot_id；另一套 navbridge 状态使用 device_id |
| fsm_state、暂停/恢复业务回包、file_progress、海康结果 | 有些无 timestamp；不能强制全部协议具有同一公共字段 |

## 2. Web 命令完整目录

表中“最终结果”表示源码实际发送或依赖的事件，不表示硬件已验收。建图/定位具体步骤见 [01_WORKFLOWS.md](01_WORKFLOWS.md)。

| type | 输入字段 | 前置条件、动作与最终结果 |
| --- | --- | --- |
| get_fsm_state | 无其他业务字段 | 回 fsm_state，全状态快照 |
| start_mapping | 无其他业务字段 | Idle；清理旧建图产物，fork mapping.sh；ack_start_mapping |
| stop_and_uploadmap | 顶层 address、display_name | Mapping；停 SLAM、后处理、上传原图；ack_stop_and_uploadmap；仍保持 Mapping |
| nav-upload-successful | data.storage_path、data.uuid | Mapping；下载 navmap、二次处理/上传、缓存；nav-upload-successful-end、FSM Idle |
| start_navigation | 顶层 address、display_name、uuid | Idle；校验 UUID、部署地图、启动导航环境；ack_start_navigation、initial_pose_ready |
| initial_pose | 顶层 pose.position xyz、pose.orientation xyzw | 导航环境 ready；发布初始位姿；等日志，ack_initial_pose，20 秒超时 |
| navigation_task | 顶层 target_points、times、frame_id、task_id、uuid、map_id | 导航环境 ready；发循环次数和目标点；等每点接收日志，ack_navigation_task，10 秒超时 |
| start_task | 无其他业务字段 | 导航环境和 FSM 定位均 ready；调用 /start_task；fsm_state；CLI 失败另有同名失败消息 |
| pause_task | 无其他业务字段 | 导航环境 ready；按当前暂停标志幂等或调用 toggle 服务；同名结果 |
| resume_task | 无其他业务字段 | 同上；恢复任务，不是重新发送航点 |
| clear_waypoints | 无其他业务字段 | 导航环境 ready；/clear_waypoints，等清除日志；ack_clear_waypoints，CLI 失败另有同名消息 |
| cancel_task | 无其他业务字段 | 导航模式，可不要求 env ready；/cancel_task；FSM 任务 Cancelled；CLI 失败另有同名消息 |
| set_dock_pose | data 或顶层：pose/position/orientation/frame_id | 导航环境 ready；latched 发布 /dock_pose；ack_set_dock_pose |
| start_recharge | 无其他业务字段 | 导航环境和定位 ready；/start_recharge；fsm_state，后续 recharge_status；CLI 失败另有同名消息 |
| stop_recharge | 无其他业务字段 | 导航模式；/stop_recharge；成功发 fsm_state，CLI 失败另有同名消息；表示结束充电/返回任务流程，不等于立即停止行走 |
| stop_navigation | 无其他业务字段 | 拒绝 Mapping/Following/StartingFollow/StoppingNavigation；清理导航、失效待确认请求；ack_stop_navigation、fsm_state |
| start_follow | 无其他业务字段 | 顶层 FSM 跟随占位接口，可抢占导航并清理；只回状态，不调用 HTTP 跟随引擎 |
| stop_follow | 无其他业务字段 | 顶层 FSM 跟随占位停止；回 fsm_state |
| follow | 顶层 cmd=start_follow/stop_follow/get_follow/estop_follow；其他字段透传 | 真正 HTTP 跟随代理，绕过上述 FSM；回 type=follow、ok、msg 等 |
| action_control / control | 顶层 command；速度在 parameters 或 data.parameters，见下文 | 运控转 /motion_cmd；一般只有 received，没有执行完成 ACK |
| set_position_upload_frequency | 顶层或 data.frequency_hz | 修改 /localization 上报限频；ack_set_position_upload_frequency |
| hikvision | 顶层或 data：command/commond、mode、angle_deg/angle | 云台相对转角或连续控制；同名 success/failed |
| hikvision_continuous | 顶层或 data.command/commond | 连续云台；同名 success/failed |
| set_param | 顶层 file_type、name、value | 修改项目 XML/YAML；错误有同名 failed，成功没有统一 ACK |
| upload_require_file | 代码不读取文件清单 | 当前只清空 map_dir，没有上传逻辑和完成回包；不能按名称当作上传接口 |
| upload_broatcast | 无其他业务字段，拼写如此 | 固定下载 trailer.mp4；download_broadcast，未实现发送给喊话器 |
| bsy_test | 无其他业务字段 | 调试：上传 /home/jianmi/yq/666.pcd 到固定 yq/000，可能有 file_progress；非正式通用接口 |

`start_add_task`、`start_add_point`、`stop_add_point`、`stop_add_task` 仅在受管命令名单/声明中保留，没有有效分发实现，不能当作已支持功能；发送后还可能占用受管命令状态。

### 2.1 导航目标点

```json
{
  "type":"navigation_task",
  "device_id":"M20-P1-bsy",
  "task_id":"task-001",
  "uuid":"MAP_UUID",
  "map_id":"map-001",
  "frame_id":"map",
  "times":1,
  "target_points":[
    {"x":1.0,"y":2.0,"z":0.0,"wait_time":3.0,"orientation":{"x":0.0,"y":0.0,"z":0.0,"w":1.0}},
    {"x":2.0,"y":2.0,"z":0.0,"wait_time":0.0,"orientation":{"x":0.0,"y":0.0,"z":0.7071068,"w":0.7071068}}
  ]
}
```

times 整数，小于 1 改为 1；frame_id 默认 map。各点 xyz 默认 0，orientation 必须提供对象，分量默认 0/0/0/1。wait_time 默认 0，单位秒；项目把它放入 `/goal.header.stamp`，作为下游约定的等待时间，**不是正常意义上的 ROS 采样时间**。

先 latched 发 `/set_nav_cycles`，等 50 ms，然后连续发 `/goal`。10 秒内收到足量 `通过话题添加航点` 才确认收到；没有逐点 ID 严格关联。空 target_points 仅记录日志并结束命令，没有统一失败 ACK。task_id/uuid/map_id 仅缓存用于进度回传，不因此自动切换导航地图。

### 2.2 充电点

```json
{"type":"set_dock_pose","device_id":"M20-P1-bsy","data":{"frame_id":"map","pose":{"position":{"x":0.5,"y":1.2,"z":0.0},"orientation":{"x":0.0,"y":0.0,"z":0.0,"w":1.0}}}}
```

也兼容顶层 data 省略、pose 省略、position 省略的扁平位置；x/y 必须存在且数值，z 默认 0。frame_id 优先 payload.frame_id，其次 payload.header.frame_id，再 map。四元数缺省 0/0/0/1。回包含 frame_id、position、orientation；只在 ROS latch 内保留，没有充电点文件持久化。

### 2.3 运控

```json
{"type":"action_control","device_id":"M20-P1-bsy","command":"move_forward","parameters":{"linear_velocity":0.2,"angular_velocity":0.0}}
```

| command | 实际输出 |
| --- | --- |
| move_forward、move_backward、turn_left、turn_right | 使用输入的有符号 linear_velocity(m/s)、angular_velocity(rad/s)；名称不会替你生成正负号，缺省速度为 0 |
| stand | target_mode=stand，速度清零，同姿态 3 秒去重 |
| down | target_mode=lie_down，速度清零，同姿态 3 秒去重 |
| stop、emergency_stop | 发布零速；没有调用硬件急停 /estop |
| emergency_stop_release、emergency_stop_force_clear | 当前只有不支持提示，没有有效解锁硬件操作 |

速度支持有限数值及完整数字字符串。Web 本接口不提供横向速度。源码实际输出 `/motion_cmd` 为 `std_msgs/String`，data 内 JSON：

```json
{"source":"navigation","target_mode":"running","ttl_sec":1.0,"twist":{"linear":{"x":0.2,"y":0.0,"z":0.0},"angular":{"x":0.0,"y":0.0,"z":0.0}}}
```

运动 target_mode 为 running，姿态为 stand/lie_down；stop/emergency_stop 保留上次 target_mode（空时用 running）并清零。command 为空但含速度字段也支持速度控制。每个有效指令至多发布一次，不是后台持续重发速度。看门狗 100 ms 检查；2 秒未收到指令则退出运动活跃标志。在收到 `/m20_v06/robot_state` 且 mode_id 不为 1/2 的前提下，空闲时每 500 ms 尝试 stand；不能据此推断任何硬件状态下都保证急停。底层另按 ttl_sec 执行，其实际实现不在本项目。

### 2.4 云台

```json
{"type":"hikvision","device_id":"M20-P1-bsy","data":{"mode":"relative","command":"left","angle_deg":15.0}}
```

```json
{"type":"hikvision_continuous","device_id":"M20-P1-bsy","data":{"command":"right"}}
```

支持 command，也兼容历史误拼 `commond`（优先于 command）。relative 为默认模式：up/down/left/right/back，angle_deg 优先 angle，数值或完整数字字符串，缺省 0；要求有限且非负。back 回零位。目标角度受设备能力范围/环绕处理约束。

连续模式可用 type=hikvision_continuous，或 hikvision + mode=continuous，也支持 keep_left/keep_right/keep_up/keep_down/keep_stop。别名 turn_left/move_left 等归一化。pan/tilt 固定速度幅值 10，zoom=0；800 ms 无更新后看门狗发零速，检查周期 100 ms，因此持续转动需要持续发指令。

结果字段：type、command、status；hikvision 还含 mode；相对模式含 angle_deg；成功可含 elevation、azimuth；失败 error_message；连续控制含 pan_speed、tilt_speed、timeout_ms，控制成功但位置读取失败可含 position_error_message。不存在统一 code/device_id/timestamp 字段保证。

ISAPI：GET `/ISAPI/PTZCtrl/channels/1/absoluteEx`，GET 对应 `/absoluteEx/capabilities`，PUT `/absoluteEx`，PUT `/continuous`。通道编号可配置；使用 XML + Digest 认证。`hikvision_ptz.py` 是独立工具，不是当前 executor 调用路径。拍照入口处于注释/未接线状态。

### 2.5 参数修改

```json
{"type":"set_param","device_id":"M20-P1-bsy","file_type":"xml","name":"closed_loop_controller/max_vx","value":0.5}
```

```json
{"type":"set_param","device_id":"M20-P1-bsy","file_type":"yaml","name":"scene","value":"indoor"}
```

XML 白名单 param_types；YAML 白名单 param_schema，完整键及类型见附录。严格校验 JSON 类型：double 需要小数值（例如 1.0，不是 1），int 需要整数，bool 需要 true/false，string 需要字符串。XML 的 max_vel/max_acc/planning_horizon 修改 arg，其余修改 param。

YAML 按 `/` 分层修改，写临时文件再 rename；并不调用 rosparam 动态更新。XML 字符串用 value.dump() 写入可能带引号。成功不回 ACK，保存/修改返回值未全部检查；无效 file_type 也可能无动作返回 true。错误消息字段误拼为 `messgae`：type=set_param、status=failed、name、expect_type、device_id、messgae。不能仅凭没收到错误就判断算法已生效。

### 2.6 两套跟随

顶层 start_follow/stop_follow 是 FSM 占位流程，与下述 `type=follow` 没有联动。

```json
{"type":"follow","device_id":"M20-P1-bsy","cmd":"get_follow","seq":1,"source":"web"}
```

| cmd | HTTP |
| --- | --- |
| start_follow | POST /follow/start，完整请求 JSON 作为 body |
| stop_follow | POST /follow/stop，完整请求 JSON 作为 body |
| get_follow | GET /follow/status |
| estop_follow | POST /follow/estop，完整请求 JSON 作为 body |

连接超时 1 秒，总超时 3 秒，响应上限 1 MiB，不重试。正常 2xx 要求响应对象有布尔 ok；非 2xx 设 ok=false、http_status，异常给 msg。保留后端字段，再设置 type=follow/device_id 并回显请求 device_id、timestamp、seq、source、cmd。真实跟随算法、状态结构取决于 8088 后端，不在本仓库；运行快照未发现该端口监听。

## 3. 回包和主动上报字段

表中列出业务字段；字段含义以状态、来源为准，不统一假设 code=0 就是完整工作流成功。

| type | 字段 / 解释 |
| --- | --- |
| fsm_state | command,status,device_id,motion_mode,task_status,health,generation,navigation_environment_ready,localization_ready,follow_placeholder,fault_reason,message,localTime。枚举见流程文档 |
| ack_start_mapping | code=0,device_id,status=success,localTime,timestamp；失败分支可能改用 type=start_mapping、code=-1/-2,message，无统一 status |
| ack_stop_and_uploadmap | device_id,display_name,address,status,storage_path,code=0,uuid,map_type=high,localTime,timestamp；status 两种 completed_*，必须检查 |
| nav-upload-successful-end | code=0,message,localTime；表示第二阶段代码执行结束，非严格文件完整性证明 |
| ack_start_navigation | code,device_id,status,localTime；多数分支另有 timestamp；成功仅表启动阶段结束。详细错误码见下面说明 |
| initial_pose_ready | timestamp,device_id,code=0/-1,status=ready/failed,localTime,message；失败 message 仍是固定就绪文字 |
| ack_initial_pose | timestamp,device_id,code=0/1,status=success/failed,localTime,message；成功须匹配 Initialize 日志 |
| ack_navigation_task | timestamp,device_id,code=0/-1,status=success/failed,confirmed_points,expected_points,localTime,message；只确认接收航点 |
| start_task/cancel_task/start_recharge/stop_recharge/clear_waypoints 的同名失败消息 | service,ret,status="执行失败",localTime；没有统一成功同名 ACK，任务/回充成功主要发 fsm_state，清点成功发 ack_clear_waypoints。ret 是 system 状态，未解析 Trigger.success |
| pause_task/resume_task 的最终同名消息 | status,code,task_state=paused/running,service,ret,message,localTime；幂等直接成功时可省略 service/ret |
| ack_clear_waypoints | timestamp,device_id,code=0/1,status=success/failed,localTime,message；靠清除日志确认 |
| ack_set_dock_pose | timestamp,device_id,code=0/1,status,localTime,message；成功另有 frame_id,position,orientation |
| ack_stop_navigation | timestamp,localTime,device_id,code=0/1,status=success/failed,message,map_id |
| ack_set_position_upload_frequency | code=0/400,status,frequency_hz,interval_ms,localTime,message(错误时)；范围 (0,1000]，数字或数字字符串 |
| upload_position | device_id,timestamp,position xyz,orientation xyzw,localTime；来自 /localization，默认最多 1 Hz，无 map_id |
| upload_path | map_id,device_id,path_id,timestamp,frame_id=map,waypoints[{position,orientation}],localTime；来自 /neupan_path，每 10 点抽样；空路径 waypoints 可能是 null |
| navigation_task_state | timestamp,device_id,code=0,event,status,message,source_message,localTime；循环完成另含 round |
| navigation_waypoint_reached | timestamp,code=0,device_id,waypoint_index,status=reached,localTime,message,task_id,uuid,map_id；有总点数时附 reached_points,total_points,progress,progress_message |
| navigation_planning_failed | timestamp,device_id,code=1,status=failed,fault_position,reason,localTime,message；若坐标解析成功另有 target xyz |
| recharge_status | timestamp,device_id,stage,event,level,status,message,localTime；是导航过程事件，不是 BMS 充电电流确认 |
| file_progress | filename,current,total,percent；current/total 字节；百分比变化或至少 1 秒更新，无 device_id/timestamp |
| nav_bridge_ping | device_id,timestamp；网关心跳，不代表定位/导航健康 |
| download_broadcast | code=0/-1,status=true/false,localTime,message；判断依赖 curl stdout 是否为空，存在误判限制 |
| hikvision/hikvision_continuous/follow/set_param | 见对应接口；没有统一公共信封 |
| ack（AI） | 原始外设 ACK 原文转发，包含 image_base64，不删除大字段 |

注意 upload_path.map_id 使用 executor 的 current_map_id，默认 map_001，不应假设它就是当前 HTTP UUID。位置回调设置的原子 localization_ready 与 FSM.localization_ready 不是同一变量，真正 start_task 准入检查后者；不要用 upload_position 替代 ack_initial_pose。

ack_start_navigation 分支：code=-1/status="Navigation environment already running"，有 message=运行检测原因；code=4/status="get uuid failed"，另有 address/display_name；code=-2/status="Missing map information"（此分支无 timestamp）；code=-1/status="Map preparation failed"；code=-2/status="Script Error: fork navigation failed"；code=0/status 为 `Map <地图名> deployed and environment started, waiting for tomogram export`。因此 code 不能脱离 status 单独判断错误类别。部分早退只 finish_command，没有把 StartingNavigation 自动置回 Idle；可查询 FSM 并通过 stop_navigation 清理。

### 3.1 任务日志与上报的完整映射

| `/rosout.msg` 子串 | 上报 |
| --- | --- |
| `Tomogram exported:` 或 `缓存地图发布完成` | 待确认环境 -> initial_pose_ready |
| `Initialize successfully!!!!!!` | 待确认初始位姿 -> ack_initial_pose 成功 |
| `通过话题添加航点` | 待确认航点计数 +1；足量后 ack_navigation_task 成功 |
| `所有航点已清除，循环次数已重置为 1` 或 `所有航点已清除` | ack_clear_waypoints 成功 |
| 同时含 `任务暂停` 和 `停止局部控制器` | navigation_task_state：task_paused / paused |
| `检测到从暂停中恢复` | navigation_task_state：task_resumed / running |
| `第N轮任务完成`（N 正整数，允许间隔空白） | navigation_task_state：task_round_completed / success，round=N |
| `到达路径点`、`到达任务点`、`达到路径点`、`达到任务点` 后首个正整数 | navigation_waypoint_reached，按索引去重 |
| `【导航预规划失败】`、`故障位置:`、`目标坐标:`、`失败原因:`，至 `任务结束: FAILURE_PLANNING` | 累计解析并发 navigation_planning_failed |

waypoint progress 按 target_points 数量计数，没有按 times 扩成总步数，也没有每轮自动重置去重索引的完整逻辑。循环完成日志本身不保证 FSM 自动变成 Succeeded。当前字符串解析没有 node/level 来源过滤，也没有与请求 ID 绑定，其他节点相同日志可能误触发。英文冒号和中文冒号不同，日志格式改变会破坏匹配。

现场源码已存在具体差异：`taskRvizServer.py:412` 打印 `任务结束：{result.message}`（中文冒号），网关等待 `任务结束: FAILURE_PLANNING`（英文冒号）。因此即使收齐前面的规划失败片段，也可能无法触发聚合上报；必须同时检查原始 `/rosout`。

### 3.2 回充全部事件

| 日志子串 | stage | event | level / status |
| --- | --- | --- | --- |
| 任务中断！进入回充逻辑... | interrupt | task_interrupted | warn / interrupted |
| 等待充电桩位姿话题 (/dock_pose)... | wait_dock_pose | waiting_dock_pose | info / waiting |
| 正在前往充电桩... | go_to_dock | going_to_dock | info / running |
| 已成功到达充电桩。 | go_to_dock | arrived_dock | info / success |
| 移动前往充电桩失败，请重新下发坐标或检查障碍物。 | go_to_dock | move_to_dock_failed | error / failed |
| 前往充电桩路径规划失败: | go_to_dock | plan_to_dock_failed | error / failed |
| 请下发一个新的、有效的充电点坐标。 | go_to_dock | need_new_dock_pose | error / failed |
| 正在充电中... 请在任务完成后点击'结束回充' | charging | charging | info / charging |
| 回充结束，正在返回任务中断点... | return_to_task | returning_to_interrupt_point | info / running |
| 已回到中断点，继续执行后续航点。 | come_to_back | come_to_back_success | info / success |
| 无法规划返回任务中断点的路径！请手动处理。 | return_to_task | return_plan_failed | error / failed |

## 4. 两种 robot_status 与电池字段

### 4.1 navbridge 业务通道 robot_status

5 Hz 定时快照，另在环境变化时立即发：type、timestamp(毫秒)、device_id、has_robot_state、has_bms_info、navigation_env_active、map、version。收到状态后才附 robot_state（`/robot_state.data` 原始字符串），收到 BMS 后才附 bms_info。

map 为 `{address,display_name}` 或空对象；version 当前代码硬编码 `M20.01.02.20260923 B`。bms_info 只有 voltage_v、soc_percent、active_alarm_levels、active_flags；告警数组元素分别 `{key,label,level}` 与 `{key,label}`。未加来源 updated_source（已注释）。navigation_env_active 只表示软件环境状态，不表示 ICP 成功。

### 4.2 BMS 独立连接 robot_status

有 BMS 数据且 WS 连通后，每 2000 ms 发：type、timestamp(秒)、robot_id、map_id="1"、current_mode="auto"、emergency_stop=false、action="stop"、cur_travel_distance=0、total_travel_distance=0、localize_status=0、relocalize_status=0、battery_level=SOC、obd_status=255、linear_velocity xyz=0、angular_velocity xyz=0、position xyz=0、orientation roll/pitch/yaw=0、error={error_code:"None",error_message:"No error"}。

除电量外，多数字段是固定占位值，**不能用 localize_status/relocalize_status 判断重定位，不能用速度/急停字段判断实际运动**。它与上一套消息同名但结构、通道不同。

### 4.3 battery_status

M20 每次 BMS 回调形成独立 battery_status；离线仅保留最新一帧，恢复后发送。字段：

| 类别 | 全部键 |
| --- | --- |
| 公共 | type,timestamp(毫秒),robot_id,source_topic |
| 基础测量 | voltage_v,current_a,soc_percent,life,power_w,total_energy_wh,mos_temp_c,board_temp_c,heat_temp_c,heat_cur_a |
| 枚举/IO | bat_state,charge_state,load_detect,do_state,di_state |
| 告警等级 | cell_overvoltage_level,cell_undervoltage_level,cell_diff_over_level,chg_high_temp_level,chg_low_temp_level,dsg_high_temp_level,dsg_low_temp_level,temp_diff_over_level,total_overvoltage_level,total_undervoltage_level,chg_overcurrent_level,dsg_overcurrent_level,soc_low_level,soh_low_level,cell_mos_overtemp_level,thermal_runaway_level |
| 标志 | smart_charger_failed,smart_load_connected,smart_load_failed,chg_mos_overtemp,chg_mos_temp_fault,dsg_mos_overtemp,dsg_mos_temp_fault,short_circuit_protect,lv_chg_forbidden,hv_dsg_forbidden,afe_fault,afe_comm_fault,afe_sample_fault,voltage_detect_fault,voltage_sampling_line_lost,total_voltage_fault,current_detect_fault,temp_detect_fault,temp_sampling_line_lost,eeprom_fault,flash_fault,rtc_fault,chg_mos_fault,dsg_mos_fault,precharge_mos_fault,precharge_failed,parallel_comm_success,parallel_comm_failed,comm_cmd_chg_mos_off,comm_cmd_dsg_mos_off,switch_chg_mos_off,switch_dsg_mos_off,fan_working,heater_working,current_limiter_working,heater_fault,smart_charger_connected |
| 组合 | alarm_flags（按源码序列排列，不是简单单值掩码）,active_alarm_levels,active_flags,alarm_info（中文告警标签合并） |

具体枚举数值的硬件定义在 robot_gateway 消息/驱动，不应自行推定。A10 需要编译时找到 bms_ros_bridge/BmsStatus.h，robot.model=A10 时订阅 /bms_status；双电池 bat01/bat02 各含 voltage_v,current_a,temperature_c,soc_percent,cycle_count,soh_percent,active_alarm_count,bat_state,chg_detect,charge_mode,alarm_flags；总消息另含 model、source_timestamp、current_peri、current_leg1..4、charge_result。SOC 取已安装/可用电池的较低值。业务 robotState 仍固定订阅 M20 话题，不会随 A10 自动完整适配。

## 5. 本地设备、AI 和灯控

云端 type=broadcast / roboticarm / ai_recognition 在 gateway 特殊分支转发**原始 JSON**给 Equipment 的所有客户端。要求匹配 device_id，使用 type/device_id/seq/id 去重；缺 seq/id 时去重函数不能形成键但仍继续转发，不是严格拒绝。此分支不经过普通命令过期检查。

业务 payload 没有在本项目定义完整模式；网关不解释机械臂动作、喊话器音量、AI 算法开关含义。不可把转发成功视为设备执行成功。机械臂/喊话器普通 ACK 未在当前 main 接回云端，只有满足下述条件的 AI ACK 被处理。

```json
{"type":"ack","device":"ai_recognition","device_id":"M20-P1-bsy","status":"ok","id":"ai-001","seq":1,"timestamp":1790000000,"result":{"algorithms":[{"type":"visual_inspection","timestamp":1790000000,"image_base64":"BASE64_IMAGE"}]}}
```

只处理文本帧；type/device/status/device_id 必须匹配，result 必须对象、algorithms 必须数组。先建图片目录，再把 ACK 原文回传云端；逐算法解码图片，不因某一图片失败撤销已转发 ACK。支持 data URL 前缀、去空白的 Base64；通过 magic bytes 选 png/jpg/gif/webp，否则 bin。文件名 `${safe_algorithm_type}_${algorithm.timestamp或ack.timestamp或0}_${index}.${ext}`，保存 `/home/jianmi/yq`；同名覆盖，id/seq 当前未用于文件名，不做完整图像解码校验。无单独本地 ACK 响应。

灯光 1 Hz 广播，状态优先级：BMS 故障 -> SOC<low_battery_percent（默认20）-> 跟随 FSM -> 导航 FSM -> Default。value 分别 bms_error、low_battery、follow、navigation、Default。Mapping/Fault 没有专属灯态。缺 BMS 不推断低电；没有样本过期判断。

**当前源码格式问题已用同一 JSON 头文件编译复现**：main.cc 的构造缺了一层 timestamp 键值对花括号，实际输出如下数组，而非预期对象：

```json
[["type","light_controller"],["value","Default"],"timestamp",1790000000]
```

本次仅记录，没有修改业务代码；板子运行进程映像已 deleted，因此是否正在发送该格式还需对正在运行的 WS 做另行实测。不要照历史文档假设当前源码会输出 `{type,value,timestamp}` 对象。

## 6. ROS 接口

队列和 latch 表示本项目 advertise/subscribe 设置；声明存在不一定有实际发布。话题可受 ROS remap 影响。

### 6.1 本项目发布

| 话题 | 类型 | 队列 / latch | 实际用途 |
| --- | --- | --- | --- |
| /initialpose | geometry_msgs/PoseWithCovarianceStamped | 1 / 是（C++ 声明） | 当前业务真正经外部 publish_initial_pose.py 单次发布，不走这个 C++ publisher；脚本不是该 latched publisher |
| /goal | geometry_msgs/PoseStamped | 10 / 否 | 下发导航航点，stamp 被约定为等待时长 |
| /dock_pose | geometry_msgs/PoseStamped | 1 / 是 | 充电点 |
| /set_nav_cycles | std_msgs/Int32 | 1 / 是 | 循环次数 |
| /navigation_loops | std_msgs/Int8 | 1 / 是 | 声明保留，当前任务路径未使用 |
| /navigation_wait_times | std_msgs/Int32MultiArray | 1 / 是 | 声明保留，当前任务路径未使用 |
| /control_mode | std_msgs/Int8 | 10 / 否 | 声明保留，当前运动改用 motion_cmd |
| /motion_cmd | std_msgs/String | 10 / 否 | 字符串内包含 source/target_mode/ttl_sec/twist JSON |

### 6.2 本项目订阅

| 话题 | 类型 | 队列 | 用途 |
| --- | --- | --- | --- |
| /localization | nav_msgs/Odometry | 10 | 位置上报；不能独自确认 FSM 定位成功 |
| /neupan_path | nav_msgs/Path | 10 | 抽样路径上报；名字保留 Neupan 不代表脚本启动了 Neupan |
| /rosout | rosgraph_msgs/Log | 50 | msg 字符串驱动业务确认、进度、回充和异常 |
| /robot_state | std_msgs/String | 10 | 原始状态字符串 |
| /m20/bms_state | robot_gateway/M20BmsState | 10 | robotState 固定订阅；BmsForwarder 默认也订阅，可配置 |
| /m20_v06/robot_state | robot_gateway/M20V06RobotState | 10 | 运控空闲站立逻辑，读取 mode_id |
| /bms_status | bms_ros_bridge/BmsStatus | 10 | 仅编译支持 A10 且配置型号 A10 |
| /tf、/tf_static | tf2_msgs/TFMessage（tf listener） | tf 库管理 | 存在 TF 监听器；不能因此推定所有备用 TF 上报函数已启动 |

### 6.3 调用的服务

| 服务 | 类型 / 提供者 | Web 入口 |
| --- | --- | --- |
| /start_task | std_srvs/Trigger，taskRvizServer | start_task |
| /toggle_task_state_service | std_srvs/Trigger，taskRvizServer | pause_task/resume_task |
| /clear_waypoints | std_srvs/Trigger，taskRvizServer | clear_waypoints |
| /cancel_task | std_srvs/Trigger，taskRvizServer | cancel_task |
| /start_recharge | std_srvs/Trigger，navActionServer | start_recharge |
| /stop_recharge | std_srvs/Trigger，navActionServer | stop_recharge |

通过 `timeout 10s rosservice call <服务>` 外部进程调用，而不是类型化 C++ service client；system 返回 0 只说明 rosservice CLI 退出成功，没有解析服务响应 success/message。下游内部另有 `/toggle_task_state`（std_srvs/Trigger）及 `/get_global_plan`（自定义 pct_planner/GetPlan，不能替换为 nav_msgs/GetPlan）；不属于本项目新增 Web 命令。

#### 下游直接关联的 action/service 定义

已读取板子 `pct_planner_ros/action/*.action` 和 `srv/GetPlan.srv`。action 自动派生 goal/cancel/status/feedback/result 等 ROS 通道，业务网关不直接发布这些 action 消息。

| 名称与类型 | 调用关系 | 字段 |
| --- | --- | --- |
| /get_global_plan，pct_planner/GetPlan | navActionServer 调用规划服务 | request：geometry_msgs/PoseStamped start、goal；response：nav_msgs/Path plan、bool success。没有 message 字段 |
| /navActionServer，pct_planner/MultiWaypointNavigationAction | taskRvizServer -> navActionServer | Goal：int16 cycles_times、geometry_msgs/PoseStamped[] waypoints；Result：bool success、string message |
| /local_follow，pct_planner/LocalFollowAction | navActionServer -> 局部跟随服务端 | Goal：nav_msgs/Path path；Result：bool success、string message |

MultiWaypointNavigation Feedback 全字段：geometry_msgs/PoseStamped current_goal；float32 distance_to_current_goal；geometry_msgs/PoseStamped final_goal；float32 total_distance_to_final_goal；string segment_type；string status；uint32 remaining_goals；float32 v；float32 w。定义注释中的 status 是 PLANNING/NAVIGATING/PAUSED/RECOVERING；距离单位米，v 为 m/s，w 为 rad/s。

LocalFollow Feedback 全字段：geometry_msgs/PoseStamped robot_pose、track_point；string segment_type；float32 v、w。

### 6.4 外部算法流程相关话题

| 话题 | 类型 | 用途 |
| --- | --- | --- |
| /map | sensor_msgs/PointCloud2 | 定位全局地图 |
| /cloud_registered | sensor_msgs/PointCloud2 | FAST LIO 当前扫描 |
| /Odometry | nav_msgs/Odometry | FAST LIO 局部里程计，注意大写 O |
| /map_to_odom | nav_msgs/Odometry | ICP 输出全局修正 |
| /cur_scan_in_map、/submap | sensor_msgs/PointCloud2 | ICP 可视化 |
| /localization | nav_msgs/Odometry | transform_fusion 输出到 map 的位姿 |
| /local_path | nav_msgs/Path | 规划路径 |
| /cmd_vel、/my_planner_vel | geometry_msgs/Twist | 下游速度链路，网关运控仍写 /motion_cmd |

frame 约定主要是 map、odom、base_link；实际 TF 需在现场核对，不能把硬编码 frame_id 直接当作坐标变换已正确完成。

## 7. 地图 HTTP 全部接口

| 方法与路径 | 请求 | 返回/使用方式 |
| --- | --- | --- |
| GET /sn_add | query sn=设备号 | 启动时注册 SN；此路径未统一设置总超时 |
| POST /request_upload | JSON type=upload_map,device_id,address,display_name,gaopei="gaopei" | JSON status、storage_path(绝对)、relative_path、uuid；status=exists 当失败返回空路径 |
| POST /get_uuid | JSON map_type="high",address,display_name | JSON uuid 或 error；curl 总超时 10 秒 |
| GET /get_map_info | query address,display_name,gaopei=gaopei，URL 编码 | JSON storage_path，后续作为 remote_dir |
| GET /health | 无 | uploader.connect 探测；探测失败当前仍可返回 true，非严格连通证明 |
| POST /upload/{remote_dir}/{filename} | 路径按实现 URL 编码，query gaopei=gaopei；body 为 application/octet-stream 原文件 | HTTP 成功表示该文件上传返回成功，不做校验和确认 |
| GET /download_file | query remote_dir,filename，编码 | 二进制直接写本地文件 |
| GET /download_broadcast/trailer.mp4 | 固定文件 | curl -f -L 保存到 /home/jianmi/yq/trailer.mp4 |

地图 uploader 连接超时 5 秒、总超时 1800 秒、不走代理；上传最多 5 次尝试，失败间隔 2 秒；下载没有同等重试，直接覆盖本地，失败可能留下半文件，无原子 rename/校验和。多文件 download_files 只要成功数量大于 0 就返回 true，并不要求全部成功。MapManager 元数据 API 使用另一套 httplib 路径，不能把 uploader 的超时设置自动套给所有请求。

文件目录/UUID 是接口约定的一部分：首轮上传 `gloabl_map.pcd,gloabl_map.pickle`，第二阶段依赖外部 `navmap.pcd` 并上传 `lightweight_map.npy,navmap.pickle`，导航缓存通常四个文件 `gloabl_map.pcd,navmap.pcd,lightweight_map.npy,navmap.pickle`。首次全局图 pickle 与导航 navmap pickle 用途不同。

## 8. 内部 C++ 接口边界

| 类/组件 | 对接入口及职责 |
| --- | --- |
| gateway | setup、set_device_id、setMessageCallback、setBroadcastCallback、setConnectedCallback、sendToWeb、isWebConnected、get_state_name；网络生命周期 |
| CommandExecutor | execute/robot_bt、setGateway、send_to_cloud、get_behavior_state；业务协调；完整声明/定义索引见附录 |
| RobotFsm | snapshot、begin_navigation/navigation_ready、localization_ready、begin_mapping/mapping、begin_stop/stopped、跟随状态、task_state；纯内存状态 |
| MappingHandle | handle_start_mapping、stop_and_uploadmap_async；通过回调接日志/网络/finish_command |
| MapManager / HttpMapUploader | upload_map、upload_map_sh、download_and_deploy、upload_files、download_files、progress_callback；地图元数据与字节流 |
| MotionHandle | handle_action_control；运控消息生成、姿态去重、看门狗 |
| LocalWebsocketServer | start、stop、broadcast、setMessageHandler；Equipment/Test 两个 service |
| AiRecognitionAckHandler | handle(payload,binary)；ACK 筛选、转发、图片落盘 |
| BmsForwarder | setBatteryCallback；BMS 独立 WS、灯态输入 |
| robotState | ROS 回调、update_navigation_environment、set_navigation_env_active、clear_navigation_environment；快照 |
| ModifyHandle | updateParam/getParam/setParam/modifyYaml/save；项目配置文件编辑 |
| HikvisionHandle / Follow / Broadcast | handle_command/handle_continuous_command、handle_follow_function、download_broadcast；设备与外部 HTTP |

这些 C++ 方法不是额外的 Web/ROS 公共 API；集成时以命令分发与实际通信边界为准。所有源码函数定位、协议 type 文字与 JSON 字段出现位置均附在参考文件中，便于后续逐项维护。
