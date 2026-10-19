# 参数与配置参考

生成日期：2026-10-19。来自板子实际源码快照；此文的参数表自动提取 C++ 白名单，配置按原文附录，不推定模板已生效。接口说明见 [02_INTERFACES.md](02_INTERFACES.md)，生效路径见 [03_OPERATIONS.md](03_OPERATIONS.md)。

double 必须使用 JSON 小数，bool 必须布尔，int 必须整数，string 必须字符串。白名单包含某键不保证目标文件中存在该键，更不保证算法实时读取。

## XML 可修改参数（90 项）

来源：src/modifyValue/modifyHandle.cc 的 param_types。

| name | JSON 类型 | 白名单源码行 |
| --- | --- | --- |
| `max_vel` | double | 47 |
| `max_acc` | double | 48 |
| `planning_horizon` | double | 49 |
| `grid_map/sensor_type` | string | 51 |
| `grid_map/cloud_is_world` | bool | 52 |
| `grid_map/need_extrinsic` | bool | 53 |
| `fsm/navi_mode` | int | 55 |
| `fsm/thresh_replan` | double | 56 |
| `fsm/thresh_no_replan` | double | 57 |
| `fsm/planning_horizon` | double | 58 |
| `fsm/emergency_time_` | double | 59 |
| `fsm/fail_safe` | bool | 60 |
| `fsm/max_replan_fail_count` | int | 61 |
| `fsm/is_real_world` | bool | 62 |
| `fsm/enable_rotation_check` | bool | 64 |
| `fsm/rot_check_len_front` | double | 65 |
| `fsm/rot_check_len_back` | double | 66 |
| `fsm/rot_check_half_width` | double | 67 |
| `fsm/rot_check_max_sweep` | double | 68 |
| `fsm/redline_crash_dist` | double | 70 |
| `fsm/redline_half_width` | double | 71 |
| `fsm/redline_step` | double | 72 |
| `grid_map/resolution` | double | 74 |
| `grid_map/sliding_map_size_x` | double | 75 |
| `grid_map/sliding_map_size_y` | double | 76 |
| `grid_map/sliding_map_size_z` | double | 77 |
| `grid_map/map_sliding_thresh` | double | 78 |
| `grid_map/double_cylinder_radius` | double | 79 |
| `grid_map/double_cylinder_offset` | double | 80 |
| `grid_map/body_height` | double | 81 |
| `grid_map/obstacles_inflation_z_up` | double | 82 |
| `grid_map/obstacles_inflation_z_down` | double | 83 |
| `grid_map/cx` | double | 85 |
| `grid_map/cy` | double | 86 |
| `grid_map/fx` | double | 87 |
| `grid_map/fy` | double | 88 |
| `grid_map/depth_filter_maxdist` | double | 90 |
| `grid_map/depth_filter_mindist` | double | 91 |
| `grid_map/depth_filter_margin` | int | 92 |
| `grid_map/k_depth_scaling_factor` | double | 93 |
| `grid_map/skip_pixel` | int | 94 |
| `grid_map/p_hit` | double | 96 |
| `grid_map/p_miss` | double | 97 |
| `grid_map/p_min` | double | 98 |
| `grid_map/p_max` | double | 99 |
| `grid_map/p_occ` | double | 100 |
| `grid_map/max_ray_length` | double | 101 |
| `grid_map/vis_height` | double | 102 |
| `grid_map/decay_enable` | bool | 104 |
| `grid_map/decay_time_thresh` | double | 105 |
| `grid_map/decay_rate` | double | 106 |
| `manager/max_vel` | double | 108 |
| `manager/max_acc` | double | 109 |
| `manager/max_jerk` | double | 110 |
| `manager/control_points_distance` | double | 111 |
| `manager/feasibility_tolerance` | double | 112 |
| `manager/planning_horizon` | double | 113 |
| `optimization/lambda_smooth` | double | 115 |
| `optimization/lambda_collision` | double | 116 |
| `optimization/lambda_feasibility` | double | 117 |
| `optimization/lambda_fitness` | double | 118 |
| `optimization/dist0` | double | 119 |
| `optimization/enable_narrow_adaptation` | bool | 120 |
| `optimization/max_vel` | double | 121 |
| `optimization/vel_tolerance` | double | 122 |
| `optimization/max_acc` | double | 123 |
| `optimization/acc_tolerance` | double | 124 |
| `closed_loop_controller/time_forward` | double | 126 |
| `closed_loop_controller/heading_error_threshold` | double | 127 |
| `closed_loop_controller/kp_pos` | double | 128 |
| `closed_loop_controller/kp_yaw` | double | 129 |
| `closed_loop_controller/max_vx` | double | 130 |
| `closed_loop_controller/max_vy` | double | 131 |
| `closed_loop_controller/max_vyaw` | double | 132 |
| `closed_loop_controller/finish_dist` | double | 133 |
| `closed_loop_controller/box_front` | double | 135 |
| `closed_loop_controller/box_back` | double | 136 |
| `closed_loop_controller/box_half_width` | double | 137 |
| `closed_loop_controller/box_t_step` | double | 138 |
| `closed_loop_controller/enable_y_velocity` | bool | 140 |
| `closed_loop_controller/deadzone_vx` | double | 142 |
| `closed_loop_controller/deadzone_vy` | double | 143 |
| `closed_loop_controller/deadzone_vyaw` | double | 144 |
| `closed_loop_controller/robot_max_vx` | double | 146 |
| `closed_loop_controller/robot_max_vy` | double | 147 |
| `closed_loop_controller/robot_max_vyaw` | double | 148 |
| `closed_loop_controller/brake_back_duration` | double | 150 |
| `closed_loop_controller/brake_back_vx` | double | 151 |
| `closed_loop_controller/stair_max_vyaw` | double | 152 |
| `closed_loop_controller/stair_max_vx` | double | 153 |

## YAML 可修改参数（49 项）

来源：src/modifyValue/modifyHandle.cc 的 param_schema。

| name | JSON 类型 | 白名单源码行 |
| --- | --- | --- |
| `scene` | string | 158 |
| `ros/odom_topic` | string | 160 |
| `ros/goal_topic` | string | 161 |
| `ros/path_topic` | string | 162 |
| `ros/dock_topic` | string | 163 |
| `ros/map_frame` | string | 165 |
| `ros/base_frame` | string | 166 |
| `ros/pointcloud_topic` | string | 168 |
| `ros/layer_G_topic` | string | 169 |
| `ros/layer_C_topic` | string | 170 |
| `ros/tomogram_topic` | string | 171 |
| `pct_planner/use_quintic` | bool | 173 |
| `pct_planner/max_heading_rate` | double | 174 |
| `tomography/resolution` | double | 176 |
| `tomography/slice_dh` | double | 177 |
| `tomography/ground_h` | double | 178 |
| `tomography/kernel_size` | int | 180 |
| `tomography/interval_min` | double | 181 |
| `tomography/interval_free` | double | 182 |
| `tomography/slope_max` | double | 184 |
| `tomography/step_max` | double | 185 |
| `tomography/standable_ratio` | double | 187 |
| `tomography/cost_barrier` | double | 188 |
| `tomography/safe_margin` | double | 189 |
| `tomography/inflation` | double | 190 |
| `nav/timeout` | double | 192 |
| `nav/default_dwell_time` | double | 193 |
| `nav/sim_initial_pose/x` | double | 195 |
| `nav/sim_initial_pose/y` | double | 196 |
| `nav/sim_initial_pose/z` | double | 197 |
| `nav/sim_initial_pose/yaw` | double | 198 |
| `nav/robot_center_offset` | double | 200 |
| `nav/external_timeout` | double | 201 |
| `nav/max_vx` | double | 203 |
| `nav/max_vy` | double | 204 |
| `nav/max_w` | double | 205 |
| `nav/switch_distance` | double | 207 |
| `nav/enable_start_align` | bool | 209 |
| `nav/align_angle_thresh` | double | 210 |
| `nav/align_done_thresh` | double | 211 |
| `nav/align_lookahead_dist` | double | 212 |
| `nav/align_stable_time` | double | 213 |
| `nav/precision_dist_thresh` | double | 215 |
| `nav/arrive_dist_thresh` | double | 216 |
| `nav/arrive_angle_thresh` | double | 217 |
| `nav/rdp_epsilon` | double | 219 |
| `nav/step_z_thresh` | double | 220 |
| `nav/stair_window_size` | int | 221 |
| `nav/slope_thresh_deg` | double | 222 |

## 配置原文与实际文件

以下是读取时的原文，包括原有注释。注释中的说明不额外代表算法行为已经验证。$(arg ...) 表达式由 ROS launch 展开，不是最终数值；实际算法配置与项目可修改模板分别列出。

### 当前启动工作目录 YAML

`/home/jianmi/grebotMap_jm/build/robot.yaml`

```yaml
app:
  name: "robot_dog"
  environment: "ceshi"
  version: "M20.01.02.20260923 B"

robot:
  sn: "M20-P1-bsy"
  model: "M20" # M20 / A10

network:
  http:
    ip: "112.2.6.99"
    port: 8503

  gateway:
    host: "0.0.0.0" # websocket 修改ip
    port: 8080
    protocol: "ws"

  endpoints:
    register: "/robot/ws/robotDog"
    navbridge: "/default"

udp:
  port: 19001

websocket:
  ip: "0.0.0.0"
  port: 9002
  server_equipment: "/robot/ws/superstructure/equirpment"
  server_test: "/robot/ws/bsy"

parameter:
  xml_path: "/home/jianmi/grebotMap_jm/advanced_param.xml"
  yaml_path: "/home/jianmi/grebotMap_jm/params.yaml"

light:
  low_battery_percent: 20 # 最低报警电量
```

### 项目根目录历史 YAML（缺 websocket 配置）

`/home/jianmi/grebotMap_jm/robot.yaml`

```yaml
app:
  name: "robot_dog"
  environment: "ceshi"
  version: "M20.01.02.20260923 B"

robot:
  sn: "M20-P1-bsy"
  model: "M20"

network:
  http:
    ip: "122.192.33.182"
    port: 2800

  gateway:
    host: "122.192.33.182" # websocket 修改ip
    port: 8598
    protocol: "ws"

  endpoints:
    register: "/robot/ws/robotDog"
    navbridge: "/robot/ws/device/nav_bridge"

udp:
  port: 19001

parameter:
  xml_path: "/home/jianmi/grebotMap_jm/advanced_param.xml"
  yaml_path: "/home/jianmi/grebotMap_jm/params.yaml"
```

### set_param 目标 PCT YAML 模板

`/home/jianmi/grebotMap_jm/params.yaml`

```yaml
# PCT Planner ROS Configuration
scene: "navmap"  # 场景/地图类型标识符，此处使用 "fused_map"

# ROS interface parameters
ros:
  # Planner topics (规划器话题)
  odom_topic: "/localization"           # 机器人里程计 (Odometry) 输入话题
  goal_topic: "/goal"           # 接收目标点 (Goal) 输入话题
  path_topic: "/global_path"    # 发布全局路径 (Global Path) 输出话题
  dock_topic: "/dock_pose"              # 【新增】自动回充/充电桩目标位姿话题
  
  # Frame IDs (坐标系 ID)
  map_frame: "map"              # 地图坐标系 ID
  base_frame: "base_link"       # 机器人基座坐标系 ID
  
  # Tomography/Sensing topics (层析/感知话题)
  pointcloud_topic: "/pointcloud" # 原始点云 (Point Cloud) 输入话题
  layer_G_topic: "/layer_G"       # 地面层 (Ground Layer) 输出话题
  layer_C_topic: "/layer_C"       # 障碍物层 (Clearance Layer) 输出话题
  tomogram_topic: "/tomogram"     # 层析图/成本图 (Tomogram/Costmap) 输出话题

# Planner parameters (规划器参数)
pct_planner:
  use_quintic: true         # 是否使用五次多项式 (Quintic) 进行轨迹生成 (通常用于平滑度更好的路径)
  max_heading_rate: 10.0    # 机器人最大航向角变化率 (Max Heading Rate) [rad/s]

# Tomography parameters (层析/成本图参数)
tomography:
  resolution:  0.11  #0.08          # 层析图/成本图的分辨率 [m/cell]
  slice_dh: 0.5             # 成本图切片的高度间隔 (Slice Height Interval) [m]
  ground_h: -10.0             # 地面高度参考值 (Ground Height Reference) [m]

  kernel_size: 7            # 用于处理点云或生成层析图的卷积核/滤波器尺寸 (Kernel Size) [cells]
  interval_min: 0.50        # 点云垂直间隔最小值 (Vertical Interval Minimum) [m]
  interval_free: 0.4       # 认为无障碍的垂直间隔 (Vertical Interval for Free Space) [m]

  slope_max: 0.55           # 允许的最大坡度 (Maximum Slope) [rad or dimensionless]
  step_max: 0.6            # 允许的最大台阶高度 (Maximum Step Height) [m]
  
  standable_ratio: 0.05    # 可站立区域的最小比例 (Minimum Standable Ratio)
  cost_barrier: 50.0        # 障碍物屏障成本 (Cost Barrier) (高于此值的区域视为不可通行)
  safe_margin: 0.2         # 机器人安全裕度 (Safe Margin) [m] (障碍物周围的额外缓冲距离)
  inflation: 0.1            # 成本图膨胀半径 (Inflation Radius) [m] (将障碍物成本向外扩展的距离)
# ==============================================================================
# 导航调度与局部控制参数 (nav 命名空间)
# ==============================================================================
nav:
  # -------------------- 全局调度与超时 --------------------
  timeout: 1200.0                       # 单段路径跟踪的超时上限阈值 [s]
  default_dwell_time: 1.0               # 当航点时间戳异常或未设定停留时间时的默认等待时长 [s]

  # -------------------- 仿真/初始默认位姿 (resetNavState使用) --------------------
  sim_initial_pose:
    x: -1.68                            # 仿真初始位姿 X 轴坐标 [m]
    y: 7.80                             # 仿真初始位姿 Y 轴坐标 [m]
    z: -6.92                            # 仿真初始位姿 Z 轴高度 [m]
    yaw: 0.0                            # 仿真初始航向角 Yaw [rad]

  # -------------------- 机器人机构尺寸与接口 --------------------
  robot_center_offset: 0.385            # 里程计测量中心(如头部传感器)到机器人几何中心/旋转中心的纵向偏置 L [m]
  external_timeout: 0.5                 # 外部局部规划器 (/my_planner_vel) 速度指令超时判定时间 [s]

  # -------------------- 运行速度限幅 (原地转向与终点精调) --------------------
  max_vx: 0.6                           # 本地控制器输出的最大前进/后退线速度 [m/s]
  max_vy: 0.6                           # 本地控制器输出的最大横向平移线速度 (四足横移特性) [m/s]
  max_w: 0.6                            # 本地控制器输出的最大旋转角速度 [rad/s]

  # -------------------- 路径段切换 --------------------
  switch_distance: 0.4                  # 当到达当前分段终点小于该距离时，触发切换至下一段 [m]

  # -------------------- 起点原地对齐控制 (Start Alignment) --------------------
  enable_start_align: false             # 起点原地旋转对齐总开关 (true: 开启, false: 关闭)
  align_angle_thresh: 0.7               # 触发原地对齐的角度误差阈值 (约 40°)，大于此值进入转向模式 [rad]
  align_done_thresh: 0.3                # 原地对齐判定完成的角度误差阈值 (约 17°) [rad]
  align_lookahead_dist: 0.5             # 原地对齐时在路径前方寻找参考目标点的预瞄距离 [m]
  align_stable_time: 0.8                # 航向达标后需要保持静止观察的稳定时长，防止振荡 [s]

  # -------------------- 终点精准到点控制 (Precision Control) --------------------
  precision_dist_thresh: 0.3            # 距离最终航点小于此值时，脱离外部规划器接管为全向精调模式 [m]
  arrive_dist_thresh: 0.25              # 终点位置容差 (平移误差小于此值即判定到达) [m]
  arrive_angle_thresh: 0.2              # 终点角度容差 (航向误差小于约 11.5° 判定朝向达标) [rad]

  # -------------------- 路径分段与楼梯识别算法 --------------------
  rdp_epsilon: 0.00001                  # RDP 抽稀算法精度阈值，越小保留点越密集 [m]
  step_z_thresh: 0.015                  # 相邻两点高度差阈值，大于此值认为存在垂直高度突变 [m]
  stair_window_size: 4                  # 连续台阶判定窗口 (连续 N 个区间高度超标才判定为楼梯)
  slope_thresh_deg: 4.0                 # 判定为坡道的俯仰角阈值 [deg]
```

### 备用 params22.yaml（当前 main 未加载）

`/home/jianmi/grebotMap_jm/params22.yaml`

```yaml
# PCT Planner ROS Configuration
scene: "navmap"  # 场景/地图类型标识符，此处使用 "fused_map"

# ROS interface parameters
ros:
  # Planner topics (规划器话题)
  odom_topic: "/localization"           # 机器人里程计 (Odometry) 输入话题
  goal_topic: "/goal"           # 接收目标点 (Goal) 输入话题
  path_topic: "/global_path"    # 发布全局路径 (Global Path) 输出话题
  dock_topic: "/dock_pose"              # 【新增】自动回充/充电桩目标位姿话题
  
  # Frame IDs (坐标系 ID)
  map_frame: "map"              # 地图坐标系 ID
  base_frame: "base_link"       # 机器人基座坐标系 ID
  
  # Tomography/Sensing topics (层析/感知话题)
  pointcloud_topic: "/pointcloud" # 原始点云 (Point Cloud) 输入话题
  layer_G_topic: "/layer_G"       # 地面层 (Ground Layer) 输出话题
  layer_C_topic: "/layer_C"       # 障碍物层 (Clearance Layer) 输出话题
  tomogram_topic: "/tomogram"     # 层析图/成本图 (Tomogram/Costmap) 输出话题

# Planner parameters (规划器参数)
pct_planner:
  use_quintic: true         # 是否使用五次多项式 (Quintic) 进行轨迹生成 (通常用于平滑度更好的路径)
  max_heading_rate: 10.0    # 机器人最大航向角变化率 (Max Heading Rate) [rad/s]

# Tomography parameters (层析/成本图参数)
tomography:
  resolution:  0.11  #0.08          # 层析图/成本图的分辨率 [m/cell]
  slice_dh: 0.5             # 成本图切片的高度间隔 (Slice Height Interval) [m]
  ground_h: -10.0             # 地面高度参考值 (Ground Height Reference) [m]

  kernel_size: 7            # 用于处理点云或生成层析图的卷积核/滤波器尺寸 (Kernel Size) [cells]
  interval_min: 0.50        # 点云垂直间隔最小值 (Vertical Interval Minimum) [m]
  interval_free: 0.4       # 认为无障碍的垂直间隔 (Vertical Interval for Free Space) [m]

  slope_max: 0.55           # 允许的最大坡度 (Maximum Slope) [rad or dimensionless]
  step_max: 0.6            # 允许的最大台阶高度 (Maximum Step Height) [m]
  
  standable_ratio: 0.05    # 可站立区域的最小比例 (Minimum Standable Ratio)
  cost_barrier: 50.0        # 障碍物屏障成本 (Cost Barrier) (高于此值的区域视为不可通行)
  safe_margin: 0.2         # 机器人安全裕度 (Safe Margin) [m] (障碍物周围的额外缓冲距离)
  inflation: 0.1            # 成本图膨胀半径 (Inflation Radius) [m] (将障碍物成本向外扩展的距离)
# ==============================================================================
# 导航调度与局部控制参数 (nav 命名空间)
# ==============================================================================
nav:
  # -------------------- 全局调度与超时 --------------------
  timeout: 1200.0                       # 单段路径跟踪的超时上限阈值 [s]
  default_dwell_time: 1.0               # 当航点时间戳异常或未设定停留时间时的默认等待时长 [s]

  # -------------------- 仿真/初始默认位姿 (resetNavState使用) --------------------
  sim_initial_pose:
    x: -1.68                            # 仿真初始位姿 X 轴坐标 [m]
    y: 7.80                             # 仿真初始位姿 Y 轴坐标 [m]
    z: -6.92                            # 仿真初始位姿 Z 轴高度 [m]
    yaw: 0.0                            # 仿真初始航向角 Yaw [rad]

  # -------------------- 机器人机构尺寸与接口 --------------------
  robot_center_offset: 0.385            # 里程计测量中心(如头部传感器)到机器人几何中心/旋转中心的纵向偏置 L [m]
  external_timeout: 0.5                 # 外部局部规划器 (/my_planner_vel) 速度指令超时判定时间 [s]

  # -------------------- 运行速度限幅 (原地转向与终点精调) --------------------
  max_vx: 0.6                           # 本地控制器输出的最大前进/后退线速度 [m/s]
  max_vy: 0.6                           # 本地控制器输出的最大横向平移线速度 (四足横移特性) [m/s]
  max_w: 0.6                            # 本地控制器输出的最大旋转角速度 [rad/s]

  # -------------------- 路径段切换 --------------------
  switch_distance: 0.4                  # 当到达当前分段终点小于该距离时，触发切换至下一段 [m]

  # -------------------- 起点原地对齐控制 (Start Alignment) --------------------
  enable_start_align: false             # 起点原地旋转对齐总开关 (true: 开启, false: 关闭)
  align_angle_thresh: 0.7               # 触发原地对齐的角度误差阈值 (约 40°)，大于此值进入转向模式 [rad]
  align_done_thresh: 0.3                # 原地对齐判定完成的角度误差阈值 (约 17°) [rad]
  align_lookahead_dist: 0.5             # 原地对齐时在路径前方寻找参考目标点的预瞄距离 [m]
  align_stable_time: 0.8                # 航向达标后需要保持静止观察的稳定时长，防止振荡 [s]

  # -------------------- 终点精准到点控制 (Precision Control) --------------------
  precision_dist_thresh: 0.3            # 距离最终航点小于此值时，脱离外部规划器接管为全向精调模式 [m]
  arrive_dist_thresh: 0.25              # 终点位置容差 (平移误差小于此值即判定到达) [m]
  arrive_angle_thresh: 0.2              # 终点角度容差 (航向误差小于约 11.5° 判定朝向达标) [rad]

  # -------------------- 路径分段与楼梯识别算法 --------------------
  rdp_epsilon: 0.00001                  # RDP 抽稀算法精度阈值，越小保留点越密集 [m]
  step_z_thresh: 0.015                  # 相邻两点高度差阈值，大于此值认为存在垂直高度突变 [m]
  stair_window_size: 4                  # 连续台阶判定窗口 (连续 N 个区间高度超标才判定为楼梯)
  slope_thresh_deg: 4.0                 # 判定为坡道的俯仰角阈值 [deg]
```

### set_param 目标 SCAN XML 模板

`/home/jianmi/grebotMap_jm/advanced_param.xml`

```xml
<launch>
    <arg name="is_real_world"/>
    <arg name="navi_mode"/>
    <arg name="sensor_type"/>
    <arg name="body_pose_topic"/>
    <arg name="sensor_pose_topic"/>
    <arg name="cloud_topic"/>
    <arg name="cloud_is_world"/>
    <arg name="depth_topic"/>
    <arg name="cx"/>
    <arg name="cy"/>
    <arg name="fx"/>
    <arg name="fy"/>
    <!-- public parameters -->
    <arg name="max_vel" default="0.7"/>
    <arg name="max_acc" default="0.4"/>
    <arg name="planning_horizon" default="5.0"/>
    <!-- 从全局上截取多长作为局部优化的目标。-->
    <!-- main node -->
    <node pkg="scan_planner" name="scan_planner_node" type="scan_planner_node" output="screen" clear_params="true">
        <remap from="/grid_map/body_pose" to="$(arg body_pose_topic)"/>
        <remap from="/grid_map/sensor_pose" to="$(arg sensor_pose_topic)"/>
        <param name="grid_map/sensor_type" value="$(arg sensor_type)" type="str"/>
        <remap from="/grid_map/cloud" to="$(arg cloud_topic)"/>
        <param name="grid_map/cloud_is_world" value="$(arg cloud_is_world)"/>
        <remap from="/grid_map/depth" to="$(arg depth_topic)"/>
        <param name="grid_map/need_extrinsic" value="$(arg cloud_is_world)" type="bool"/>
        <!-- planning fsm -->
        <param name="fsm/navi_mode" value="$(arg navi_mode)" type="int"/>
        <param name="fsm/thresh_replan" value="0.2" type="double"/>
        <!-- 触发重新规划的位移阈值 -->
        <param name="fsm/thresh_no_replan" value="0.1" type="double"/>
        <!--  临近终点停止规划的阈值 -->
        <param name="fsm/planning_horizon" value="$(arg planning_horizon)" type="double"/>
        <param name="fsm/emergency_time_" value="1.0" type="double"/>
        <!--  如果预测1s内会撞上且无法避障，则EMERGENCY_STOP -->
        <param name="fsm/fail_safe" value="true" type="bool"/>
        <!--  安全开关。开启后，如果规划连续失败会强制停止 -->
        <param name="fsm/max_replan_fail_count" value="1000" type="int"/>
        <!--  允许规划失败的最大次数 -->
        <param name="fsm/is_real_world" value="$(arg is_real_world)" type="bool"/>
        <!-- 新增：长方形机身旋转避障配置 -->
        <param name="fsm/enable_rotation_check" value="true" type="bool"/>
        <!-- 开关：true开启，false关闭 -->
        <param name="fsm/rot_check_len_front" value="0.45" type="double"/>
        <!-- 中心到车头距离 (m) -->
        <param name="fsm/rot_check_len_back" value="0.45" type="double"/>
        <!-- 中心到车尾距离 (m) -->
        <param name="fsm/rot_check_half_width" value="0.24" type="double"/>
        <!-- 机身半宽 (m，总宽0.36m) -->
        <param name="fsm/rot_check_max_sweep" value="0.8" type="double"/>
        <!-- 预测旋转角最大检测范围 (rad，约35度) -->
        <!-- 空间物理绝对红线参数 (贴脸急停) -->
        <param name="fsm/redline_crash_dist" value="0.10" type="double"/>
        <!-- 前方致命急停红线距离 (m) -->
        <param name="fsm/redline_half_width" value="0.15" type="double"/>
        <!-- 红线检测区域半宽 (m) -->
        <param name="fsm/redline_step" value="0.05" type="double"/>
        <!-- 红线前向采样步长 (m) -->
        <!-- sliding map -->
        <param name="grid_map/resolution" value="0.05"/>
        <!-- 每个栅格的大小 -->
        <param name="grid_map/sliding_map_size_x" value="10.0"/>
        <!-- 滑动窗口地图的大小 -->
        <param name="grid_map/sliding_map_size_y" value="10.0"/>
        <param name="grid_map/sliding_map_size_z" value="5.0"/>
        <param name="grid_map/map_sliding_thresh" value="0.2"/>
        <!-- 地图更新触发位移 -->
        <param name="grid_map/double_cylinder_radius" value="0.24"/>
        <!-- 宽 ≈ 2 * 半径-->
        <param name="grid_map/double_cylinder_offset" value="0.21"/>
        <!-- 长 ≈ 2 * 偏移量 + 2 * 半径-->
        <param name="grid_map/body_height" value="0.62"/>
        <param name="grid_map/obstacles_inflation_z_up" value="0.4"/>
        <!-- 处理地面碰撞安全距离-->
        <param name="grid_map/obstacles_inflation_z_down" value="0.4"/>
        <!-- 限高碰撞安全距离  0.35   0.45height-->
        <!-- depth camera intrinsics -->
        <param name="grid_map/cx" value="$(arg cx)"/>
        <param name="grid_map/cy" value="$(arg cy)"/>
        <param name="grid_map/fx" value="$(arg fx)"/>
        <param name="grid_map/fy" value="$(arg fy)"/>
        <!-- depth -->
        <param name="grid_map/depth_filter_maxdist" value="3.0"/>
        <param name="grid_map/depth_filter_mindist" value="0.3"/>
        <param name="grid_map/depth_filter_margin" value="9"/>
        <param name="grid_map/k_depth_scaling_factor" value="1000.0"/>
        <param name="grid_map/skip_pixel" value="2"/>
        <!-- local fusion -->
        <param name="grid_map/p_hit" value="0.85"/>
        <!-- 传感器观测到障碍物时的置信度增加值 0.85  0.9-->
        <param name="grid_map/p_miss" value="0.35"/>
        <!--未观测到障碍物时的置信度减小值  0.30-->
        <param name="grid_map/p_min" value="0.12"/>
        <!-- 栅格占据概率的上下限及判定为“占据”的阈值 -->
        <param name="grid_map/p_max" value="0.98"/>
        <!--栅格占据概率的上下限及判定为“占据”的阈值  -->
        <param name="grid_map/p_occ" value="0.8"/>
        <!-- 判定为“占据”的阈值0.80 -->
        <param name="grid_map/max_ray_length" value="6.0"/>
        <!-- 超过 5m 的传感器数据将被忽略或处理为自由空间 -->
        <param name="grid_map/vis_height" value="0.25" type="double"/>
        <!-- 可视化高度截断阈值 -->
        <!-- 新增：时间衰减参数配置 -->
        <param name="grid_map/decay_enable" value="true" type="bool"/>
        <param name="grid_map/decay_time_thresh" value="4.0" type="double"/>
        <!-- 1.0秒未观测到则判定离开视野/盲区 -->
        <param name="grid_map/decay_rate" value="2.0" type="double"/>
        <!-- 衰减速率，5大约0.4s内彻底清除 -->
        <!-- planner manager -->
        <param name="manager/max_vel" value="$(arg max_vel)" type="double"/>
        <!-- 轨迹允许的最大速度 -->
        <param name="manager/max_acc" value="$(arg max_acc)" type="double"/>
        <!--  -->
        <param name="manager/max_jerk" value="4" type="double"/>
        <!--  -->
        <param name="manager/control_points_distance" value="0.5" type="double"/>
        <!-- B样条控制点之间的平均距离。点越密轨迹越灵活但计算量越大。-->
        <param name="manager/feasibility_tolerance" value="0.5" type="double"/>
        <!-- 物理可行性松弛。检查轨迹是否超速时允许的溢出比例  -->
        <param name="manager/planning_horizon" value="$(arg planning_horizon)" type="double"/>
        <!--  -->
        <!-- trajectory optimization -->
        <param name="optimization/lambda_smooth" value="1.0" type="double"/>
        <!-- 轨迹平滑度权重。越大，轨迹越像直线  1.0，窄道0.6-0.8-->
        <param name="optimization/lambda_collision" value="1.5" type="double"/>
        <!--  避障权重。越大，轨迹离障碍物越远。 -->
        <param name="optimization/lambda_feasibility" value="0.2" type="double"/>
        <!--动力学可行性权重 0.1不动 -->
        <param name="optimization/lambda_fitness" value="1.0" type="double"/>
        <!-- 轨迹贴合度权重。越大，生成的局部轨迹越贴合原始全局路径。 -->
        <param name="optimization/dist0" value="0.2" type="double"/>
        <!-- 避障安全距离阈值（0.2m）。距障碍物小于此值时会产生斥力。窄道0.15 -->
        <param name="optimization/enable_narrow_adaptation" value="false" type="bool"/>
        <!-- 窄道自适应切换开关: true开启 / false关闭 -->
        <param name="optimization/max_vel" value="$(arg max_vel)" type="double"/>
        <!--  -->
        <param name="optimization/vel_tolerance" value="1.0" type="double"/>
        <!--  -->
        <param name="optimization/max_acc" value="$(arg max_acc)" type="double"/>
        <!--  -->
        <param name="optimization/acc_tolerance" value="1.0" type="double"/>
        <!--  -->
    </node>
    <!-- trajectory tracking -->
    <param name="closed_loop_controller/time_forward" value="0.1" type="double"/>
    <!-- 用于计算机器人当前应该朝向的方向  0.9-->
    <param name="closed_loop_controller/heading_error_threshold" value="0.8" type="double"/>
    <!-- 航向角误差阈值（0.8 rad） -->
    <param name="closed_loop_controller/kp_pos" value="0.8" type="double"/>
    <!--  0.8-->
    <param name="closed_loop_controller/kp_yaw" value="1.0" type="double"/>
    <!-- 转向纠偏力度 (P控制参数) -->
    <param name="closed_loop_controller/max_vx" value="$(arg max_vel)" type="double"/>
    <!--  -->
    <param name="closed_loop_controller/max_vy" value="0.8" type="double"/>
    <!--  -->
    <param name="closed_loop_controller/max_vyaw" value="0.8" type="double"/>
    <!--  -->
    <param name="closed_loop_controller/finish_dist" value="0.2" type="double"/>
    <!-- 到达判定距离 -->
    <!-- 控制器朝向推算的车身包围盒及搜索步长 -->
    <param name="closed_loop_controller/box_front" value="0.20" type="double"/>
    <!-- 中心到车头距离 (m) -->
    <param name="closed_loop_controller/box_back" value="0.20" type="double"/>
    <!-- 中心到车尾距离 (m) -->
    <param name="closed_loop_controller/box_half_width" value="0.20" type="double"/>
    <!-- 车身半宽 (m) -->
    <param name="closed_loop_controller/box_t_step" value="0.05" type="double"/>
    <!-- 前瞻轨迹时间搜索步长 (s) -->
    <!-- 是否使能Y轴平移速度：true=全向移动(默认), false=仅使用X和Yaw -->
    <param name="closed_loop_controller/enable_y_velocity" value="true" type="bool"/>
    <!-- 新增：机器人的死区速度（启动所需最小速度），根据底盘测出来填 -->
    <param name="closed_loop_controller/deadzone_vx" value="0.2" type="double"/>
    <param name="closed_loop_controller/deadzone_vy" value="0.1" type="double"/>
    <param name="closed_loop_controller/deadzone_vyaw" value="0.0" type="double"/>
    <!-- 新增：底层机器人对应的物理最大速度 -->
    <!-- (如果不填，代码里默认它等于上面设置的算法 max_vx / max_vyaw) -->
    <param name="closed_loop_controller/robot_max_vx" value="0.7" type="double"/>
    <param name="closed_loop_controller/robot_max_vy" value="0.8" type="double"/>
    <param name="closed_loop_controller/robot_max_vyaw" value="0.8" type="double"/>
    <!-- 新增：急停后退配置 (持续时间 / 后退速度) -->
    <!-- 若不想后退直接停死，将 duration 或 vx 改为 0.0 即可 -->
    <param name="closed_loop_controller/brake_back_duration" value="0.0" type="double"/>
    <!-- 后退持续秒数 -->
    <param name="closed_loop_controller/brake_back_vx" value="-0.0" type="double"/>
    <!-- 后退速度 (负数 m/s) -->
    <!-- ==================== 新增：楼梯直行与安全参数（免编译可调） ==================== -->
    <!-- 楼梯上的最大旋转角速度 (rad/s)，建议 0.15~0.25，防止大幅度甩头 -->
    <param name="closed_loop_controller/stair_max_vyaw" value="0.5" type="double"/>
    <!-- 楼梯上的最大前进速度 (m/s)，建议放慢保证平稳 -->
    <param name="closed_loop_controller/stair_max_vx" value="0.5" type="double"/>
</launch>
```

### PCT launch 实际加载的 YAML

`/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/config/params.yaml`

```yaml
# PCT Planner ROS Configuration
scene: "navmap"  # 场景/地图类型标识符，此处使用 "fused_map"

# ROS interface parameters
ros:
  # Planner topics (规划器话题)
  odom_topic: "/localization"           # 机器人里程计 (Odometry) 输入话题
  goal_topic: "/goal"           # 接收目标点 (Goal) 输入话题
  path_topic: "/global_path"    # 发布全局路径 (Global Path) 输出话题
  
  # Frame IDs (坐标系 ID)
  map_frame: "map"              # 地图坐标系 ID
  base_frame: "base_link"       # 机器人基座坐标系 ID
  
  # Tomography/Sensing topics (层析/感知话题)
  pointcloud_topic: "/pointcloud" # 原始点云 (Point Cloud) 输入话题
  layer_G_topic: "/layer_G"       # 地面层 (Ground Layer) 输出话题
  layer_C_topic: "/layer_C"       # 障碍物层 (Clearance Layer) 输出话题
  tomogram_topic: "/tomogram"     # 层析图/成本图 (Tomogram/Costmap) 输出话题

# Planner parameters (规划器参数)
pct_planner:
  use_quintic: true         # 是否使用五次多项式 (Quintic) 进行轨迹生成 (通常用于平滑度更好的路径)
  max_heading_rate: 10.0    # 机器人最大航向角变化率 (Max Heading Rate) [rad/s]

# Tomography parameters (层析/成本图参数)
tomography:
  resolution:  0.1  #0.08          # 层析图/成本图的分辨率 [m/cell]
  slice_dh: 0.5             # 成本图切片的高度间隔 (Slice Height Interval) [m]
  ground_h: -10.0             # 地面高度参考值 (Ground Height Reference) [m]

  kernel_size: 7            # 用于处理点云或生成层析图的卷积核/滤波器尺寸 (Kernel Size) [cells]
  interval_min: 0.50        # 点云垂直间隔最小值 (Vertical Interval Minimum) [m]
  interval_free: 0.4       # 认为无障碍的垂直间隔 (Vertical Interval for Free Space) [m]

  slope_max: 0.55           # 允许的最大坡度 (Maximum Slope) [rad or dimensionless]
  step_max: 0.6            # 允许的最大台阶高度 (Maximum Step Height) [m]
  
  standable_ratio: 0.05    # 可站立区域的最小比例 (Minimum Standable Ratio)
  cost_barrier: 50.0        # 障碍物屏障成本 (Cost Barrier) (高于此值的区域视为不可通行)
  safe_margin: 0.2         # 机器人安全裕度 (Safe Margin) [m] (障碍物周围的额外缓冲距离)
  inflation: 0.1            # 成本图膨胀半径 (Inflation Radius) [m] (将障碍物成本向外扩展的距离)

nav:
  switch_distance: 0.3      # 到达目标点的容忍距离 (Goal Tolerance) [m]
  ORIENTATION_THRESHOLD: 0.15 # 目标方向容忍阈值 (Orientation Threshold) [rad] 约5.7度
  enable_start_align: false    # 【新增】起点原地旋转对齐使能 (true: 开启, false: 关闭)
```

### SCAN launch 实际加载的 XML

`/home/jianmi/catkin_scan/src/SCAN-Planner-main/src/planner/plan_manage/launch/advanced_param.xml`

```xml
<launch>
  <arg name="is_real_world"/>
  <arg name="navi_mode"/>
  <arg name="sensor_type"/>
  <arg name="body_pose_topic"/>
  <arg name="sensor_pose_topic"/>
  <arg name="cloud_topic"/>
  <arg name="cloud_is_world"/>
  <arg name="depth_topic"/>
  <arg name="cx"/>
  <arg name="cy"/>
  <arg name="fx"/>
  <arg name="fy"/>

  <!-- public parameters -->
  <arg name="max_vel" default="0.7"/>
  <arg name="max_acc" default="0.4"/>
  <arg name="planning_horizon" default="5.0"/>  <!-- 从全局上截取多长作为局部优化的目标。-->

  <!-- main node -->
  <node pkg="scan_planner" name="scan_planner_node" type="scan_planner_node" output="screen" clear_params="true">
    <remap from="/grid_map/body_pose" to="$(arg body_pose_topic)"/>
    <remap from="/grid_map/sensor_pose" to="$(arg sensor_pose_topic)"/>
    <param name="grid_map/sensor_type" value="$(arg sensor_type)" type="str"/>
    <remap from="/grid_map/cloud" to="$(arg cloud_topic)"/>
    <param name="grid_map/cloud_is_world" value="$(arg cloud_is_world)"/>
    <remap from="/grid_map/depth" to="$(arg depth_topic)"/>
    <param name="grid_map/need_extrinsic" value="$(arg cloud_is_world)" type="bool"/>

    <!-- planning fsm -->
    <param name="fsm/navi_mode" value="$(arg navi_mode)" type="int"/>
    <param name="fsm/thresh_replan" value="0.2" type="double"/>    <!-- 触发重新规划的位移阈值 -->
    <param name="fsm/thresh_no_replan" value="0.1" type="double"/>  <!--  临近终点停止规划的阈值 -->
    <param name="fsm/planning_horizon" value="$(arg planning_horizon)" type="double"/>
    <param name="fsm/emergency_time_" value="1.5" type="double"/>   <!--  如果预测1s内会撞上且无法避障，则EMERGENCY_STOP -->
    <param name="fsm/fail_safe" value="true" type="bool"/>   <!--  安全开关。开启后，如果规划连续失败会强制停止 -->
    <param name="fsm/max_replan_fail_count" value="1000" type="int"/>   <!--  允许规划失败的最大次数 -->
    <param name="fsm/is_real_world" value="$(arg is_real_world)" type="bool"/>

    <!-- sliding map -->
    <param name="grid_map/resolution" value="0.05" />   <!-- 每个栅格的大小 -->
    <param name="grid_map/sliding_map_size_x" value="10.0" />   <!-- 滑动窗口地图的大小 -->
    <param name="grid_map/sliding_map_size_y" value="10.0" /> 
    <param name="grid_map/sliding_map_size_z" value="5.0" />  
    <param name="grid_map/map_sliding_thresh" value="0.2"/>   <!-- 地图更新触发位移 -->
    <param name="grid_map/double_cylinder_radius" value="0.3" />  <!-- 宽 ≈ 2 * 半径-->
    <param name="grid_map/double_cylinder_offset" value="0.32" />  <!-- 长 ≈ 2 * 偏移量 + 2 * 半径-->
    <param name="grid_map/body_height" value="0.45" />
    <param name="grid_map/obstacles_inflation_z_up" value="0.2" /> <!-- 处理天花板碰撞安全距离-->
    <param name="grid_map/obstacles_inflation_z_down" value="0.35" /> <!-- 处理地面的碰撞安全距离-->

    <!-- depth camera intrinsics -->
    <param name="grid_map/cx" value="$(arg cx)"/>
    <param name="grid_map/cy" value="$(arg cy)"/>
    <param name="grid_map/fx" value="$(arg fx)"/>
    <param name="grid_map/fy" value="$(arg fy)"/>

    <!-- depth -->
    <param name="grid_map/depth_filter_maxdist"   value="3.0"/>
    <param name="grid_map/depth_filter_mindist"   value="0.3"/>
    <param name="grid_map/depth_filter_margin"    value="1"/>
    <param name="grid_map/k_depth_scaling_factor" value="1000.0"/>
    <param name="grid_map/skip_pixel" value="2"/>

    <!-- local fusion -->
    <param name="grid_map/p_hit" value="0.85"/>   <!-- 传感器观测到障碍物时的置信度增加值 0.85  0.9-->
    <param name="grid_map/p_miss" value="0.35"/>   <!--未观测到障碍物时的置信度减小值  0.30-->
    <param name="grid_map/p_min" value="0.12"/>   <!-- 栅格占据概率的上下限及判定为“占据”的阈值 -->
    <param name="grid_map/p_max" value="0.98"/>   <!--栅格占据概率的上下限及判定为“占据”的阈值  -->
    <param name="grid_map/p_occ" value="0.8"/>   <!-- 判定为“占据”的阈值0.80 -->
    <param name="grid_map/max_ray_length" value="5.0"/>   <!-- 超过 5m 的传感器数据将被忽略或处理为自由空间 -->
    <param name="grid_map/vis_height" value="0.3" type="double"/>   <!-- 可视化高度截断阈值 -->
    
    <!-- 新增：时间衰减参数配置 -->
    <param name="grid_map/decay_enable" value="true" type="bool"/>
    <param name="grid_map/decay_time_thresh" value="4.0" type="double"/> <!-- 1.0秒未观测到则判定离开视野/盲区 -->
    <param name="grid_map/decay_rate" value="2.0" type="double"/>        <!-- 衰减速率，5大约0.4s内彻底清除 -->

    <!-- planner manager -->
    <param name="manager/max_vel" value="$(arg max_vel)" type="double"/>   <!-- 轨迹允许的最大速度 -->
    <param name="manager/max_acc" value="$(arg max_acc)" type="double"/>   <!--  -->
    <param name="manager/max_jerk" value="4" type="double"/>   <!--  -->
    <param name="manager/control_points_distance" value="0.5" type="double"/> <!-- B样条控制点之间的平均距离。点越密轨迹越灵活但计算量越大。-->
    <param name="manager/feasibility_tolerance" value="0.5" type="double"/>   <!-- 物理可行性松弛。检查轨迹是否超速时允许的溢出比例  -->
    <param name="manager/planning_horizon" value="$(arg planning_horizon)" type="double"/>   <!--  -->

    <!-- trajectory optimization -->
    <param name="optimization/lambda_smooth" value="0.8" type="double"/>   <!-- 轨迹平滑度权重。越大，轨迹越像直线  1.0，窄道0.6-0.8-->
    <param name="optimization/lambda_collision" value="1.5" type="double"/>   <!--  避障权重。越大，轨迹离障碍物越远。 -->
    <param name="optimization/lambda_feasibility" value="0.2" type="double"/>   <!--动力学可行性权重 0.1不动 -->
    <param name="optimization/lambda_fitness" value="0.8" type="double"/>   <!-- 轨迹贴合度权重。越大，生成的局部轨迹越贴合原始全局路径。 -->
    <param name="optimization/dist0" value="0.25" type="double"/>   <!-- 避障安全距离阈值（0.2m）。距障碍物小于此值时会产生斥力。窄道0.15 -->
        <param name="optimization/enable_narrow_adaptation" value="false" type="bool"/>  <!-- 窄道自适应切换开关: true开启 / false关闭 -->
    <param name="optimization/max_vel" value="$(arg max_vel)" type="double"/>   <!--  -->
    <param name="optimization/vel_tolerance" value="1.0" type="double"/>   <!--  -->
    <param name="optimization/max_acc" value="$(arg max_acc)" type="double"/>   <!--  -->
    <param name="optimization/acc_tolerance" value="1.0" type="double"/>   <!--  -->

  </node>

  <!-- trajectory tracking -->
  <param name="closed_loop_controller/time_forward" value="0.0" type="double"/>   <!-- 用于计算机器人当前应该朝向的方向  0.9-->
  <param name="closed_loop_controller/heading_error_threshold" value="0.8" type="double"/>   <!-- 航向角误差阈值（0.8 rad） -->
  <param name="closed_loop_controller/kp_pos" value="0.8" type="double"/>   <!--  0.8-->
  <param name="closed_loop_controller/kp_yaw" value="1.0" type="double"/>   <!-- 转向纠偏力度 (P控制参数) -->
  <param name="closed_loop_controller/max_vx" value="$(arg max_vel)" type="double"/>   <!--  -->
  <param name="closed_loop_controller/max_vy" value="0.8" type="double"/>   <!--  -->
  <param name="closed_loop_controller/max_vyaw" value="0.99" type="double"/>   <!--  -->
  <param name="closed_loop_controller/finish_dist" value="0.2" type="double"/>   <!-- 到达判定距离 -->
  <!-- 是否使能Y轴平移速度：true=全向移动(默认), false=仅使用X和Yaw -->
  <param name="closed_loop_controller/enable_y_velocity" value="true" type="bool"/>
  
  <!-- 新增：机器人的死区速度（启动所需最小速度），根据底盘测出来填 -->
  <param name="closed_loop_controller/deadzone_vx" value="0.2" type="double"/> 
  <param name="closed_loop_controller/deadzone_vy" value="0.2" type="double"/> 
  <param name="closed_loop_controller/deadzone_vyaw" value="0.0" type="double"/> 

  <!-- 新增：底层机器人对应的物理最大速度 -->
  <!-- (如果不填，代码里默认它等于上面设置的算法 max_vx / max_vyaw) -->
  <param name="closed_loop_controller/robot_max_vx" value="0.6" type="double"/> 
  <param name="closed_loop_controller/robot_max_vy" value="0.8" type="double"/> 
  <param name="closed_loop_controller/robot_max_vyaw" value="0.99" type="double"/>

</launch>
```
