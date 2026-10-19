# 部署、配置与排障

本文命令是现场操作参考。本次整理只读取项目/运行状态、生成文档及核实 JSON 序列化，没有启动建图、导航、定位、运动、回充或调用控制服务。

## 1. 目录和产物

| 路径 | 用途 |
| --- | --- |
| /home/jianmi/grebotMap_jm | C++ 源码与配置模板 |
| /home/jianmi/grebotMap_jm/build | 当前 robot_dog 工作目录；实际读取这里的 robot.yaml |
| /home/jianmi/catkin_pct/Scripts | mapping/slam/navigation/map_process/map_process2/kill_all 脚本 |
| /home/jianmi/g_build_map | 建图原始点云、合并点云、gloabl_map.pcd/pickle |
| /home/jianmi/g_nav_map | 当前导航地图暂存；多个命令会清空内容 |
| /home/jianmi/map_manager_local/UUID | UUID 地图缓存；通常四个导航文件 |
| /home/jianmi/fast_wa | FAST-LIVO2 / FAST LIO 定位工作空间，注意两个算法不同 |
| /home/jianmi/catkin_pct | PCT 规划、地图层、任务/动作服务 |
| /home/jianmi/catkin_scan | SCAN 局部规划工作空间 |
| /home/jianmi/catkin_ws、catkin_livox | 底层接口、雷达依赖环境 |
| /home/jianmi/jm/log | robot_dog 业务日志 |
| /home/jianmi/.ros/log | ROS 节点与 roslaunch 日志 |
| /home/jianmi/yq | AI 图片、trailer.mp4、调试上传文件 |

全局原始地图叫 `gloabl_map.pcd`，拼写是现有契约；导航可编辑点云叫 `navmap.pcd`；`navmap.pickle` 是导航层析/地图缓存；`lightweight_map.npy` 是轻量地图产物。不要只替换其中一个文件而默认整套地图一致。

## 2. 配置优先级与实际读取位置

### 2.1 robot.yaml

main 第一条配置读取为 `loadConfig("robot.yaml")`，取决于启动工作目录。当前应核对 `/home/jianmi/grebotMap_jm/build/robot.yaml`，不是只改项目根文件。根目录旧 YAML 缺 websocket 必需项，直接从根目录启动可能配置解析失败。

完整实际 YAML 见参数附录。字段消费关系：

| 配置键 | 用途 / 说明 |
| --- | --- |
| robot.sn | 被 YAML 解析，但 main 另取系统 SN/ROS device_id，不能保证改此字段就改设备号 |
| robot.model | BmsForwarder 型号 M20/A10；executor 另有 ROS robot_model 默认 M20，两者不自动同步 |
| network.http.ip / port | 地图 HTTP IP/端口；IP 可用 ROS http_ip 覆盖，port 无对应 main ROS 覆盖 |
| network.gateway.host / port / protocol | 组成向 Web 主动连接的地址，host 不是本地监听 IP |
| network.endpoints.navbridge | 业务 WS 路径前缀 |
| network.endpoints.register | BMS 独立 WS 路径前缀；网关心跳路径另硬编码 robotDog |
| udp.port | 配置加载但 main 没有实际启动对应 UDP 服务，不是可用 UDP 业务接口 |
| bms.model 或兼容 bsm.model | 实际作为 BMS 话题字符串，默认 /m20/bms_state，名称 model 容易误解 |
| parameter.xml_path / yaml_path | set_param 修改目标文件；不是已证明被算法加载的路径 |
| websocket.ip / port / server_equipment / server_test | 本地设备 WS 监听与路由 |
| light.low_battery_percent | 默认 20，要求 [0,100]；严格小于阈值才 low_battery |
| app.name/environment/version | 配置原文存在，但 loadConfig 未读取这些 app 字段；状态 version 在源码硬编码 |

设备号优先过程：get_serial_number() 从系统序列号文件读取（实现中通过 SSH 命令读板子自身 `/etc/serial-number`），再由 ROS device_id 参数覆盖并去首尾空白；最终不能为空。当前 SN 为 M20-P1-bsy。

### 2.2 ROS 参数

| 参数名 | 默认值 | 所属/作用 |
| --- | --- | --- |
| device_id | 系统 SN | main public NodeHandle；设备 ID |
| http_ip | YAML network.http.ip | main；地图服务 IP |
| scripts_path | /home/jianmi/catkin_pct/Scripts/ | main；部分脚本仍在源码写死绝对路径 |
| map_dir | /home/jianmi/g_nav_map | main；部分复制步骤仍写死 g_nav_map |
| bms_websocket_url | 按 YAML 拼接 register/ID | main；BMS 独立连接 |
| follow_base_url | http://127.0.0.1:8088 | executor；HTTP 跟随后端 |
| hikvision_ip | 192.168.44.64 | executor；云台 IP |
| hikvision_username | admin | executor；云台账户 |
| hikvision_password | 源码有默认凭据，本文不重复明文 | executor；通过部署参数设置 |
| hikvision_channel | 1 | executor；云台通道 |
| robot_model | M20 | executor；与 YAML 型号不同来源 |
| ~local_port | 9002 | gateway 旧 handset 服务参数；旧服务启动已注释，本地 WS 使用 YAML 端口 |
| ~ping_second | 2 | gateway；Ping 周期 |
| ~pong_timeout | 5 | gateway；Pong 超时秒数 |
| ~command_timeout_ms | 60000 | gateway；普通命令允许最大年龄，<=0 关闭年龄检查 |
| ~reconnect_max_attempts | 0 | gateway；0 无限重连 |
| ~reconnect_base_delay_ms | 1000 | gateway；首次重连基础延时 |
| ~reconnect_max_delay_ms | 60000 | gateway；重连上限 |

public NodeHandle 参数在无 namespace 的当前部署中是 `/device_id` 等；private `~` 参数在 `/leggedog_gateway_node/` 下。不能把设置 `_device_id:=...` 自动当成 public device_id 参数；实际 remap/namespace 需按 ROS 规则确认。

### 2.3 算法实际参数与 set_param 的断层

| 内容 | set_param 修改 | launch 实际加载 |
| --- | --- | --- |
| PCT YAML | /home/jianmi/grebotMap_jm/params.yaml | /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/config/params.yaml |
| SCAN XML | /home/jianmi/grebotMap_jm/advanced_param.xml | /home/jianmi/catkin_scan/src/SCAN-Planner-main/src/planner/plan_manage/launch/advanced_param.xml |

本次核对两个 YAML 文件大小分别 5974 与 2711 字节，XML 分别 11684 与 8622 字节，内容不同。SCAN run.launch 使用 `$(find scan_planner)/launch/advanced_param.xml`，PCT global_planner.launch 从自身 rsc/config 加载。项目没有自动复制、符号链接或动态 reload 的完整链路。

因此参数修改的验收顺序应是：确认目标文件变化 -> 确认被 launch 加载的路径 -> 确认节点是否读取新值 -> 在下一次经授权启动环境时复核 ROS 参数和效果。本文不擅自同步算法配置，因为这会改变机器人行为。只改 grebotMap_jm 下模板通常不足以认定生效。

## 3. 构建与启动条件

顶层 CMake 项目 robot_dog，C++17，CMake >=3.16。依赖 ROS Noetic：roscpp、geometry_msgs、nav_msgs、rosgraph_msgs、sensor_msgs、std_msgs、tf，以及系统 Threads、OpenSSL、Iconv、libcurl、tinyxml2、yaml-cpp、LibXml2、uuid、libssh2；另外使用 websocketpp/Boost、spdlog/fmt。M20 消息头来自 robot_gateway 工作空间；A10 为可选 bms_ros_bridge 消息编译支持。

顶层没有完整 catkin package.xml/launch 部署封装，按现有 CMake 构建目录工作。以下仅为维护示例，本次没有执行构建或重启：

```bash
source /opt/ros/noetic/setup.bash
source /home/jianmi/catkin_ws/devel/setup.bash
cd /home/jianmi/grebotMap_jm
cmake -S . -B build
cmake --build build -j2
cd build
./robot_dog
```

执行启动前须确认没有现有 robot_dog，避免端口冲突、多个业务连接或重复控制。导航/initial_pose 代码依赖 gnome-terminal、有效桌面会话与 DISPLAY；纯 SSH 中脚本被 fork 不表示 GUI 子进程能运行。导航还依赖脚本中各工作空间可 source，GTSAM 动态库路径存在，雷达/IMU、TF 和底层机器人驱动正常。

main 启动顺序：加载 YAML/XML -> 取 SN/初始化 ROS -> 构造 executor、运控与状态订阅 -> 启动 BMS/本地 WS -> 灯控定时器 -> HTTP SN 注册 -> 绑定网关回调/连接 -> ros::spin。SN HTTP 请求未明确设置总超时，可能使后面的连接建立延迟。

已有测试源：robot_fsm_test、light_policy_test、local_websocket_smoke、ai_ack_handler_test、follow_http_test/follow_http_mock_test。不能因为文件存在就说都执行通过，顶层 CMake 未提供一套统一 add_test 流程。本次未编译或运行机器人业务测试；只编译了独立 JSON 格式复现程序。

## 4. 现场只读诊断命令

先在板子终端准备环境：

```bash
source /opt/ros/noetic/setup.bash
source /home/jianmi/catkin_ws/devel/setup.bash
```

### 4.1 运行与配置

```bash
pgrep -af robot_dog
cat /etc/serial-number
cat /home/jianmi/grebotMap_jm/build/robot.yaml
rosnode info /leggedog_gateway_node
rosparam get /device_id
rosparam get /leggedog_gateway_node
ss -ltnp
```

ROS 参数不存在会报错，不代表代码没有默认值。用实际 PID 检查 `/proc/PID/cwd`、`/proc/PID/exe`；exe 带 `(deleted)` 表示当前进程映像已被替换/删除，不能把当前源码或磁盘文件当作该进程的精确版本。

### 4.2 重定位最直接的日志判定

```bash
rostopic echo /rosout/msg | grep --line-buffered -E 'Initialize successfully|Relocalize successfully|ICP Result|Fitness Score|Match Reject|First scan|Odometry not received|Waiting for global map|Localization node ready|Tomogram exported|缓存地图发布完成'
```

需要查看日志节点、级别和文件时：

```bash
rostopic echo /rosout
rostopic info /initialpose
rostopic info /map_to_odom
rostopic hz /Odometry
rostopic hz /cloud_registered
rostopic hz /localization
rostopic echo -n 1 /map_to_odom
rostopic echo -n 1 /localization
```

rostopic hz/echo 是持续观察，需要 Ctrl-C 结束；没有消息时可能一直等待。地图通常是 latched，不应强制要求地图频率持续大于某个值。融合 /localization 在未匹配时也可能有数据，成功仍需当前 initial_pose 的 `ack_initial_pose.code=0,status=success`；`Relocalize successfully!!!!!!` 当前不被网关接受。

### 4.3 导航与运动数据链

```bash
rostopic info /goal
rostopic echo -n 1 /set_nav_cycles
rostopic echo -n 1 /dock_pose
rostopic hz /neupan_path
rostopic echo /motion_cmd
rostopic echo -n 1 /m20_v06/robot_state
rostopic echo -n 1 /m20/bms_state
rosservice list
rosservice type /start_task
rosrun tf tf_echo map base_link
```

这里不包含 rosservice call、rostopic pub 等会触发动作的命令。Web 查询 `get_fsm_state` 是业务层查状态入口；9002 测试路由不是任意控制入口。

### 4.4 地图与日志

```bash
find /home/jianmi/g_build_map /home/jianmi/g_nav_map /home/jianmi/map_manager_local -maxdepth 2 -type f -printf '%s %p\n'
tail -n 150 /home/jianmi/jm/log/robot_2026_10_19.log
grep -nE 'InitialPose|NavigationEnv|Mapping|FSM|NavigationTask|failed|timeout' /home/jianmi/jm/log/robot_2026_10_19.log
```

上例日志日期是本次核对日，日后按实际启动日期替换。LogManager 以启动日期选择 `robot_YYYY_MM_DD.log`，使用 basic_file_sink，并不会跨午夜自动切换；启动时按修改时间清理多余日志，阈值 30（清理后再创建新文件，不能保证始终恰好最多 30 个）。

建图、后处理、kill_all 等经 script_log_command 捕获到业务日志；gnome-terminal 中的导航/初始位姿输出不保证都被捕获，需结合 ROS 日志和终端。业务日志包含命令和 AI ACK 原文，排查时注意图片 Base64 可能很大。

## 5. 故障判断表

| 症状 | 优先检查 | 当前实现导致的可能结果 |
| --- | --- | --- |
| start_mapping 收到 success 但没点云 | SLAM 子进程、mapping_avia.launch、雷达/IMU、脚本日志 | ACK 只证明 fork，不检查节点健康 |
| stop_and_uploadmap 一直不回 | 建图进程是否退出、waitpid、后处理脚本 | 等子进程退出没有超时 |
| code=0 但地图没上传 | ack_stop_and_uploadmap.status、单文件传输日志 | completed_but_upload_failed 仍 code=0 |
| 二阶段完成但导航图不完整 | 四文件大小/版本、每个下载上传结果 | 第二阶段忽略部分 bool；多文件下载只需任意成功 |
| 新图首次启动失败 | 新 UUID 缓存是否先存在 navmap.pcd/pickle | 先复制缓存再下载，可能在下载前失败 |
| 导航回了 ACK，initial_pose 被拒绝 | initial_pose_ready、FSM env_ready、Tomogram 日志 | 启动 ACK 不等于环境就绪 |
| failed 消息却写“重定位已准备就绪” | code/status，而非 message | 失败分支固定 message 文案有误 |
| initial_pose 超时，算法显示 Relocalize successfully | ICP fitness/RMSE、地图/初始姿态 | 直接接受初值分支与网关成功关键词不一致 |
| initial_pose 没有进入算法 | gnome-terminal/DISPLAY、/initialpose 订阅者、首次扫描/里程计 | 发布子进程失败也只等待日志超时 |
| 有 upload_position，却不能 start_task | get_fsm_state.localization_ready、ack_initial_pose | 原子定位变量与 FSM 定位变量不同 |
| 航点收到但机器人不动 | 是否发 start_task、Trigger.success 原响应、下游规划/速度 | 航点 ACK 只是接收，CLI 成功可掩盖业务拒绝 |
| 多轮没有后续点到达上报 | task_round_completed 与 waypoint 索引 | 网关点索引去重未逐轮复位 |
| 暂停恢复状态不同步 | toggle 服务、rosout 暂停/恢复日志、请求队列 | 部分状态靠日志，同名/迟到日志可能误触发 |
| FSM Running 但任务已结束 | 末点/round 日志、底层速度 | 缺完整任务结束到 Succeeded 的统一同步 |
| 跟随灯亮但未跟随 | 顶层 start_follow 与 type=follow 的区别 | FSM 顶层接口是占位，不调用 8088 |
| type=follow 失败 | 8088 后端是否监听、响应 ok 布尔、网络超时 | 本次运行快照无 8088 监听 |
| emergency_stop 后硬件急停未触发 | /motion_cmd 零速与硬件 /estop 链路 | 本项目没有实际调用硬件急停接口 |
| 改参数无效果 | 修改路径与 launch 实际加载路径、是否重新加载 | set_param 不自动同步/动态更新算法 |
| 灯控外设解析失败 | 接收的原始 JSON 类型 | 当前源码输出数组，已本地编译复现 |
| 电量/定位状态与 Web 显示冲突 | 哪条连接、robot_id/device_id、两种 robot_status | BMS 独立状态的定位/速度是占位值 |
| 机械臂/喊话器 ACK 没上传 | Equipment main handler | 当前只处理 AI ACK |
| WS 重连后收到旧状态 | pending 队列、timestamp、服务端去重 | 非位置消息可能积压，状态并非可靠持久消息队列 |

网关主 WS 重连首轮默认 1 秒、后续按次数增长到 60 秒上限，默认无限重连，连接成功重置次数；BMS 独立通道约 5 秒重连。网关 pending 队列与 BMS 最新单帧策略不同，没有端到端 exactly-once 或任务持久恢复机制。普通命令去重缓存与 FSM 都在内存中，进程重启后不保留。

`kMaxPendingMsgs=50` 不是所有消息共用的硬上限：网关仅对 `upload_position` 和名为 `robot_state_info` 的类型作特殊裁剪，实际业务状态类型却是 `robot_status`，其他类型仍可继续入队。长时间断线可能积压旧状态。另一个已核实的日志差异是任务节点使用 `任务结束：`，网关匹配 `任务结束:`，可能使 `navigation_planning_failed` 无法结束聚合；应直接查看 `/rosout` 原始消息。

## 6. 本次只读运行快照

时间：2026-10-19，非连续监控。

| 项目 | 实测 |
| --- | --- |
| 进程 | PID 82200，命令 ./robot_dog，cwd=/home/jianmi/grebotMap_jm/build |
| 映像 | /proc/82200/exe -> 项目 build/robot_dog (deleted) |
| SN | /etc/serial-number 为 M20-P1-bsy，/device_id 未设置，启动日志使用该 SN |
| 网关节点 | /leggedog_gateway_node 存在 |
| 其他节点 | /cmd_vel_bridge、/robot_gateway、/rosout |
| 运动状态字符串 | /robot_state.data="unknown" |
| TCP | 9002 robot_dog、8080 Java、11311 ROS master；未发现 8088 |
| 算法环境 | 查询时未发现建图/导航进程；导航控制服务未在当前列表中 |

可见话题包括 /cmd_vel、/cmd_vel_mode、/control_mode、/dock_pose、/estop、/gateway_status、/goal、/imu/data、/imu_data、/initialpose、/localization、/m20/bms_state、/m20/motor_state、/m20_v06/robot_state、/motion_cmd、/navigation_loops、/navigation_wait_times、/neupan_path、/robot_state、/rosout、/rosout_agg、/set_nav_cycles、/tf、/tf_static。仅看到话题名称不说明数据频率/质量或实际 publisher 存活。

## 7. 文档维护与已知范围

本套文档的参考附录包含所有配置白名单、业务 JSON 键出现位置、消息 type、类方法源码索引，以及读取文件的 SHA-256。主接口文档解释有效契约；源码索引用于定位，不把注释/未接线函数当作已支持功能。

需要维护的优先事项：统一重定位成功确认协议与算法回退分支；用类型化服务响应确认任务调用；按完整文件集合/校验和部署地图；修复灯控序列化；统一参数生效路径与两种 robot_status；增加定位质量/时间有效性及任务完成同步。以上仅记录发现，本次没有实施业务修复。
