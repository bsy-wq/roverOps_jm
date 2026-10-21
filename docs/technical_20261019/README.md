# grebotMap_jm 技术文档

核对日期：2026-10-19。目标板：`jianmi@192.168.44.102`。项目：`/home/jianmi/grebotMap_jm`。

本套文档根据板子上的 C++ 源码、实际调用的 Shell/Python/launch 文件、`build/robot.yaml`、ROS 只读查询整理。描述当前实现；示例是操作说明，本次整理没有执行建图、重定位、运动、导航、回充或服务调用。

## 阅读顺序

1. [建图、重定位与导航流程](01_WORKFLOWS.md)：什么时候建图、什么时候重定位、各阶段成功条件、状态机。
2. [全部业务与通信接口](02_INTERFACES.md)：Web 命令、回包、上报、ROS 话题/服务、HTTP、设备通信。
3. [部署、配置与排障](03_OPERATIONS.md)：启动环境、参数来源、现场命令、代码限制及运行快照。
4. [参数和配置参考](04_REFERENCE.md)：全部 XML/YAML 可修改参数，附项目配置和算法实际配置原文。
5. [源码和消息字段索引](05_SOURCE_INDEX.md)：命令、type、JSON 字段、C++ 方法、外部脚本原文与快照 SHA-256。

## 最关键的判定

| 看到的内容 | 实际含义 |
| --- | --- |
| `status: received` | 网关已接收请求，操作尚未确认成功 |
| `ack_start_mapping.code=0` | 已 fork 建图脚本，不保证 SLAM 已收到雷达数据 |
| `ack_stop_and_uploadmap.status=completed_and_uploaded` | 第一阶段地图处理和上传返回成功，建图流程尚未结束 |
| `nav-upload-successful-end.code=0` | 代码执行到二次处理和缓存落盘末尾，FSM 回到 Idle；仍存在未严格校验下载/上传返回值的限制 |
| `initial_pose_ready.status=ready, code=0` | 导航地图已加载/导出，可下发初始位姿；还没有完成重定位 |
| `/rosout.msg` 包含 `Initialize successfully!!!!!!` | 当前网关在存在待确认 initial_pose 请求时采用的成功判据 |
| `ack_initial_pose.status=success, code=0` | 网关已经确认本次重定位成功，FSM 的 localization_ready 变为 true |
| `/rosout.msg` 包含 `Relocalize successfully!!!!!!` | 算法放宽处理、直接接受初始位姿的分支；网关当前不识别，可能仍超时失败 |
| `/localization` 有数据或 Web 有 `upload_position` | 有位姿输出，不足以单独证明 ICP 配准成功 |
| `ack_navigation_task.status=success` | 下游已按日志确认收到目标点，不代表已经走完任务 |

你提到的“roscout”在本项目中对应 ROS 的 `/rosout`，消息类型是 `rosgraph_msgs/Log`，核心判断字段为 `msg`。

## 证据范围

源代码快照保存在本机 `../source`（板子文档目录没有另复制源码）。项目内源码使用 `src/...`、`include/...` 标注；项目外依赖使用板子绝对路径标注。本地外部依赖快照位于 `../source/home/jianmi/...`。

运行检查时 `robot_dog` PID 为 82200，工作目录为项目的 `build`，`/proc/82200/exe` 显示 `(deleted)`，说明运行中的程序映像与当前磁盘可执行文件不能直接视为同一个版本。本文的行为结论以本次读取的源码为依据，运行查询只证明当时的部署状态，未进行硬件闭环验收。

`FSM_DESIGN.md`、`LOCAL_WEBSOCKET.md`、`LIGHT_CONTROL.md`、`MOTION_WEB_CONTROL_PROTOCOL.md` 是已有历史文档，其中部分内容落后于源码；本套文档列出差异，不覆盖这些旧文件。
