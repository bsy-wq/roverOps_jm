# 源码、消息字段与外部脚本索引

生成日期：2026-10-19。所有路径以板子快照为准。字段与函数扫描限定 src/include，排除第三方 JSON/HTTP 头文件、历史 Qt 示例和测试。本文件按文本提取定位信息，含注释/备用函数可能出现的字面量，不能代替有效接口契约；是否接线以 [02_INTERFACES.md](02_INTERFACES.md) 为准。

本机原始快照位于本文目录的 ../source；同步到板子的这套文档没有另复制源码，项目内路径在 /home/jianmi/grebotMap_jm 下，home/jianmi 开头的路径对应板子绝对路径。

## 业务命令分发的全部 type 比较

| 命令 | 分发源码位置 |
| --- | --- |
| `initial_pose` | src/CommandExecutor.cc:534, 695 |
| `start_navigation` | src/CommandExecutor.cc:535, 580, 645, 684, 703 |
| `navigation_task` | src/CommandExecutor.cc:536, 711 |
| `stop_navigation` | src/CommandExecutor.cc:537, 667, 739 |
| `start_mapping` | src/CommandExecutor.cc:538, 689, 748 |
| `stop_and_uploadmap` | src/CommandExecutor.cc:539, 761 |
| `nav-upload-successful` | src/CommandExecutor.cc:540, 770 |
| `start_task` | src/CommandExecutor.cc:541, 779 |
| `clear_waypoints` | src/CommandExecutor.cc:542, 795 |
| `cancel_task` | src/CommandExecutor.cc:543, 816 |
| `start_add_task` | src/CommandExecutor.cc:544 |
| `start_add_point` | src/CommandExecutor.cc:545 |
| `stop_add_point` | src/CommandExecutor.cc:546 |
| `stop_add_task` | src/CommandExecutor.cc:547 |
| `get_fsm_state` | src/CommandExecutor.cc:654 |
| `start_follow` | src/CommandExecutor.cc:658 |
| `stop_follow` | src/CommandExecutor.cc:658 |
| `pause_task` | src/CommandExecutor.cc:663, 787, 794 |
| `resume_task` | src/CommandExecutor.cc:663, 787 |
| `start_recharge` | src/CommandExecutor.cc:664, 824 |
| `stop_recharge` | src/CommandExecutor.cc:664, 832 |
| `set_dock_pose` | src/CommandExecutor.cc:665, 719 |
| `action_control` | src/CommandExecutor.cc:727 |
| `control` | src/CommandExecutor.cc:727 |
| `set_position_upload_frequency` | src/CommandExecutor.cc:737 |
| `upload_require_file` | src/CommandExecutor.cc:840 |
| `hikvision_continuous` | src/CommandExecutor.cc:848 |
| `hikvision` | src/CommandExecutor.cc:859 |
| `set_param` | src/CommandExecutor.cc:873 |
| `bsy_test` | src/CommandExecutor.cc:875 |
| `upload_broatcast` | src/CommandExecutor.cc:880 |
| `follow` | src/CommandExecutor.cc:882 |

## JSON type 字面量索引

覆盖初始化器、下标赋值、value 比较及示例中的 type。动态 type=cmd_type/request_type 见命令目录；设备原文转发不在此枚举固定 payload。

| type | 出现位置 |
| --- | --- |
| `ack` | `src/aiRecognition/AiRecognitionAckHandler.cc:73` |
| `ack_clear_waypoints` | `src/CommandExecutor.cc:1798` |
| `ack_initial_pose` | `src/CommandExecutor.cc:1680` |
| `ack_navigation_task` | `src/CommandExecutor.cc:1762` |
| `ack_set_dock_pose` | `src/CommandExecutor.cc:2315`、`src/CommandExecutor.cc:2364` |
| `ack_set_position_upload_frequency` | `src/CommandExecutor.cc:309`、`src/CommandExecutor.cc:325` |
| `ack_start_mapping` | `src/mapping/maphandle.cc:134` |
| `ack_start_navigation` | `src/CommandExecutor.cc:1920`、`src/CommandExecutor.cc:1945`、`src/CommandExecutor.cc:2010`、`src/CommandExecutor.cc:2088`、`src/CommandExecutor.cc:2135`、`src/CommandExecutor.cc:2191` |
| `ack_stop_and_uploadmap` | `src/mapping/maphandle.cc:250` |
| `ack_stop_navigation` | `src/CommandExecutor.cc:2513` |
| `battery_status` | `src/bms/BmsForwarder.cc:196`、`src/bms/BmsForwarder.cc:394` |
| `download_broadcast` | `src/broadcast/broadcastHandle.cc:24`、`src/broadcast/broadcastHandle.cc:34` |
| `file_progress` | `include/http_map_uploader.h:613` |
| `follow` | `src/follow/followHandle.cc:120` |
| `fsm_state` | `src/CommandExecutor.cc:890` |
| `hikvision` | `src/hikvision/hikvihandle.cc:584`、`src/hikvision/hikvihandle.cc:598` |
| `initial_pose_ready` | `src/CommandExecutor.cc:1726` |
| `light_controller` | `src/main.cc:102` |
| `nav_bridge_ping` | `src/gateway/gateway.cc:554` |
| `nav-upload-successful-end` | `src/CommandExecutor.cc:2647` |
| `navigation_planning_failed` | `src/CommandExecutor.cc:1643` |
| `navigation_task_state` | `src/CommandExecutor.cc:1294`、`src/CommandExecutor.cc:1313` |
| `navigation_waypoint_reached` | `src/CommandExecutor.cc:1414` |
| `recharge_status` | `src/CommandExecutor.cc:1273` |
| `robot_status` | `src/bms/BmsForwarder.cc:551`、`src/robotState/robotState.cc:202` |
| `set_param` | `src/modifyValue/modifyHandle.cc:480`、`src/modifyValue/modifyHandle.cc:502` |
| `start_mapping` | `src/CommandExecutor.cc:2541`、`src/mapping/maphandle.cc:71`、`src/mapping/maphandle.cc:95` |
| `stop_and_uploadmap` | `src/CommandExecutor.cc:2669` |
| `upload_map` | `include/manager.h:37` |
| `upload_path` | `src/CommandExecutor.cc:1198`、`src/CommandExecutor.cc:1152` |
| `upload_position` | `src/CommandExecutor.cc:1249`、`src/CommandExecutor.cc:1831`、`src/CommandExecutor.cc:1215` |

## JSON 字段定位

按文件列出 JSON 初始化器键、下标键、value/contains/find/at 访问键和源码行。某些字段是输入，某些是输出，少量可能是配置键/日志示例；消息结构与必选条件以业务文档和对应构造函数为准。

### include/common/utils.h

| 字段/键 | 源码行 |
| --- | --- |
| `address` | 92 |
| `display_name` | 93 |
| `error` | 111, 113 |
| `map_type` | 91 |
| `uuid` | 116 |

### include/config/yamlConfig.h

| 字段/键 | 源码行 |
| --- | --- |
| `bms` | 56, 57 |
| `bsm` | 58, 59 |
| `endpoints` | 51, 52 |
| `gateway` | 47, 48, 49 |
| `host` | 47 |
| `http` | 44, 45 |
| `ip` | 44, 67 |
| `light` | 71, 72 |
| `low_battery_percent` | 71, 72 |
| `model` | 42, 56, 57, 58, 59 |
| `navbridge` | 52 |
| `network` | 44, 45, 47, 48, 49, 51, 52 |
| `parameter` | 64, 65 |
| `port` | 45, 48, 54, 68 |
| `protocol` | 49 |
| `register` | 51 |
| `robot` | 41, 42 |
| `server_equipment` | 69 |
| `server_test` | 70 |
| `sn` | 41 |
| `udp` | 54 |
| `websocket` | 67, 68, 69, 70 |
| `xml_path` | 64 |
| `yaml_path` | 65 |

### include/http_map_uploader.h

| 字段/键 | 源码行 |
| --- | --- |
| `current` | 615 |
| `filename` | 614 |
| `percent` | 617 |
| `total` | 616 |
| `type` | 613 |

### include/manager.h

| 字段/键 | 源码行 |
| --- | --- |
| `address` | 40, 107, 136 |
| `device_id` | 38 |
| `display_name` | 39, 108, 137 |
| `gaopei` | 43, 108, 137 |
| `relative_path` | 58 |
| `status` | 52 |
| `storage_path` | 57, 114, 143 |
| `type` | 37 |
| `uuid` | 59 |

### src/CommandExecutor.cc

| 字段/键 | 源码行 |
| --- | --- |
| `active_command` | 561 |
| `address` | 1935, 1950, 2043 |
| `code` | 310, 326, 560, 1297, 1316, 1416, 1646, 1683, 1729, 1765, 1801, 1923, 1948, 2013, 2092, 2138, 2195, 2319, 2368, 2517, 2649, 2671, 2763, 2805 |
| `command` | 890 |
| `confirmed_points` | 1767 |
| `data` | 282, 283, 2292, 2293, 2557 |
| `device_id` | 622, 623, 891, 1200, 1250, 1275, 1296, 1315, 1417, 1645, 1682, 1728, 1764, 1800, 1832, 1922, 1947, 2011, 2090, 2137, 2194, 2318, 2367, 2516 |
| `display_name` | 1934, 1952, 2044 |
| `error_message` | 853, 868 |
| `event` | 1277, 1298, 1317 |
| `expected_points` | 1769 |
| `fault_position` | 1648 |
| `fault_reason` | 897 |
| `follow_placeholder` | 896 |
| `frame_id` | 1203, 2267, 2348, 2351, 2371 |
| `frequency_hz` | 288, 290, 328 |
| `generation` | 895 |
| `gloabl_map.pcd` | 2049 |
| `header` | 2350, 2351 |
| `health` | 893 |
| `interval_ms` | 329 |
| `level` | 1278 |
| `lightweight_map.npy` | 2610 |
| `localization_ready` | 896 |
| `localTime` | 312, 330, 562, 698, 706, 714, 722, 730, 742, 751, 764, 773, 782, 790, 798, 819, 827, 835, 843, 862, 897, 1204, 1253, 1279, 1301, 1321, 1420, 1650, 1685, 1731, 1768, 1803, 1834, 1925, 1951, 2012, 2091, 2139, 2193, 2317, 2366, 2515, 2542, 2648, 2670, 2764, 2804, 2852 |
| `map_id` | 1199, 1424, 2232, 2233, 2520 |
| `message` | 313, 563, 897, 1281, 1300, 1320, 1421, 1651, 1686, 1732, 1770, 1804, 1926, 2321, 2370, 2519, 2543, 2650, 2672, 2766, 2809 |
| `motion_mode` | 891 |
| `navigation_environment_ready` | 895 |
| `orientation` | 1186, 1254, 1836, 1858, 2273, 2274, 2275, 2276, 2331, 2332, 2333, 2334, 2373 |
| `path_id` | 1201 |
| `pose` | 1857, 1858, 2295, 2296 |
| `position` | 1181, 1252, 1835, 1857, 2298, 2299, 2372 |
| `progress` | 1430 |
| `progress_message` | 1431 |
| `reached_points` | 1428 |
| `reason` | 1649 |
| `ret` | 2808, 2853 |
| `robot_id` | 624, 625 |
| `round` | 1319 |
| `service` | 2807, 2850 |
| `source_message` | 1302, 1322 |
| `stage` | 1276 |
| `status` | 311, 327, 559, 699, 707, 715, 723, 731, 743, 752, 765, 774, 783, 791, 799, 820, 828, 836, 844, 852, 863, 890, 1280, 1299, 1318, 1419, 1647, 1684, 1730, 1766, 1802, 1924, 1949, 2014, 2093, 2140, 2196, 2320, 2369, 2518, 2762, 2803, 2851 |
| `storage_path` | 2559 |
| `target` | 1655 |
| `target_points` | 2215 |
| `task_id` | 1422, 2228, 2229 |
| `task_state` | 2765, 2806 |
| `task_status` | 892 |
| `times` | 2253 |
| `timestamp` | 1202, 1251, 1274, 1295, 1314, 1415, 1644, 1681, 1727, 1763, 1799, 1833, 1921, 1946, 2089, 2136, 2192, 2316, 2365, 2514 |
| `total_points` | 1429 |
| `type` | 309, 325, 558, 617, 618, 697, 705, 713, 721, 729, 741, 750, 763, 772, 781, 789, 797, 818, 826, 834, 842, 851, 861, 867, 890, 1198, 1249, 1273, 1294, 1313, 1414, 1643, 1680, 1726, 1762, 1798, 1831, 1920, 1945, 2010, 2088, 2135, 2191, 2315, 2364, 2513, 2541, 2647, 2669, 2761, 2802, 2849 |
| `uuid` | 1423, 1936, 2230, 2231, 2560 |
| `w` | 1190, 1254, 1836, 1867, 2276, 2373 |
| `wait_time` | 2266 |
| `waypoint_index` | 1418 |
| `waypoints` | 1205 |
| `x` | 1182, 1187, 1252, 1254, 1655, 1835, 1836, 1860, 1864, 2269, 2273, 2372, 2373 |
| `y` | 1183, 1188, 1252, 1254, 1655, 1835, 1836, 1861, 1865, 2270, 2274, 2372, 2373 |
| `z` | 1184, 1189, 1252, 1254, 1655, 1835, 1836, 1862, 1866, 2271, 2275, 2372, 2373 |

### src/aiRecognition/AiRecognitionAckHandler.cc

| 字段/键 | 源码行 |
| --- | --- |
| `algorithms` | 79 |
| `device` | 74 |
| `device_id` | 75 |
| `id` | 92 |
| `image_base64` | 96, 97, 100 |
| `result` | 77 |
| `seq` | 93 |
| `status` | 75 |
| `timestamp` | 106 |
| `type` | 73, 105 |

### src/bms/BmsForwarder.cc

| 字段/键 | 源码行 |
| --- | --- |
| `action` | 557 |
| `active_alarm_count` | 373 |
| `active_alarm_levels` | 269, 418 |
| `active_flags` | 270, 417 |
| `afe_comm_fault` | 242 |
| `afe_fault` | 241 |
| `afe_sample_fault` | 243 |
| `alarm_flags` | 268, 375 |
| `alarm_info` | 271 |
| `angular_velocity` | 565 |
| `bat_state` | 210, 373 |
| `bat01` | 396 |
| `bat02` | 397 |
| `battery_level` | 562 |
| `board_temp_c` | 207 |
| `cell_diff_over_level` | 217 |
| `cell_mos_overtemp_level` | 229 |
| `cell_overvoltage_level` | 215 |
| `cell_undervoltage_level` | 216 |
| `charge_mode` | 374 |
| `charge_result` | 400 |
| `charge_state` | 211 |
| `chg_detect` | 374 |
| `chg_high_temp_level` | 218 |
| `chg_low_temp_level` | 219 |
| `chg_mos_fault` | 253 |
| `chg_mos_overtemp` | 234 |
| `chg_mos_temp_fault` | 235 |
| `chg_overcurrent_level` | 225 |
| `comm_cmd_chg_mos_off` | 259 |
| `comm_cmd_dsg_mos_off` | 260 |
| `cur_travel_distance` | 558 |
| `current_a` | 201, 370 |
| `current_detect_fault` | 247 |
| `current_leg1` | 398 |
| `current_leg2` | 398 |
| `current_leg3` | 399 |
| `current_leg4` | 399 |
| `current_limiter_working` | 265 |
| `current_mode` | 555 |
| `current_peri` | 397 |
| `cycle_count` | 372 |
| `di_state` | 214 |
| `do_state` | 213 |
| `dsg_high_temp_level` | 220 |
| `dsg_low_temp_level` | 221 |
| `dsg_mos_fault` | 254 |
| `dsg_mos_overtemp` | 236 |
| `dsg_mos_temp_fault` | 237 |
| `dsg_overcurrent_level` | 226 |
| `eeprom_fault` | 250 |
| `emergency_stop` | 556 |
| `error` | 568 |
| `error_code` | 569 |
| `error_message` | 570 |
| `fan_working` | 263 |
| `flash_fault` | 251 |
| `heat_cur_a` | 209 |
| `heat_temp_c` | 208 |
| `heater_fault` | 266 |
| `heater_working` | 264 |
| `hv_dsg_forbidden` | 240 |
| `key` | 52, 63, 420 |
| `label` | 53, 64, 71, 74 |
| `level` | 54 |
| `life` | 203 |
| `linear_velocity` | 564 |
| `load_detect` | 212 |
| `localize_status` | 560 |
| `lv_chg_forbidden` | 239 |
| `map_id` | 554 |
| `model` | 395 |
| `mos_temp_c` | 206 |
| `obd_status` | 563 |
| `orientation` | 567 |
| `parallel_comm_failed` | 258 |
| `parallel_comm_success` | 257 |
| `pitch` | 567 |
| `position` | 566 |
| `power_w` | 204 |
| `precharge_failed` | 256 |
| `precharge_mos_fault` | 255 |
| `relocalize_status` | 561 |
| `robot_id` | 198, 395, 553 |
| `roll` | 567 |
| `rtc_fault` | 252 |
| `short_circuit_protect` | 238 |
| `smart_charger_connected` | 267 |
| `smart_charger_failed` | 231 |
| `smart_load_connected` | 232 |
| `smart_load_failed` | 233 |
| `soc_low_level` | 227 |
| `soc_percent` | 202, 371, 396 |
| `soh_low_level` | 228 |
| `soh_percent` | 372 |
| `source_timestamp` | 400 |
| `source_topic` | 199, 395 |
| `switch_chg_mos_off` | 261 |
| `switch_dsg_mos_off` | 262 |
| `temp_detect_fault` | 248 |
| `temp_diff_over_level` | 222 |
| `temp_sampling_line_lost` | 249 |
| `temperature_c` | 371 |
| `thermal_runaway_level` | 230 |
| `timestamp` | 197, 394, 552 |
| `total_energy_wh` | 205 |
| `total_overvoltage_level` | 223 |
| `total_travel_distance` | 559 |
| `total_undervoltage_level` | 224 |
| `total_voltage_fault` | 246 |
| `type` | 196, 394, 551 |
| `voltage_detect_fault` | 244 |
| `voltage_sampling_line_lost` | 245 |
| `voltage_v` | 200, 370 |
| `x` | 564, 565, 566 |
| `y` | 564, 565, 566 |
| `yaw` | 567 |
| `z` | 564, 565, 566 |

### src/broadcast/broadcastHandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `code` | 25, 35 |
| `localTime` | 27, 37 |
| `message` | 28, 38 |
| `status` | 26, 36 |
| `type` | 24, 34 |

### src/follow/followHandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `cmd` | 41 |
| `device_id` | 121, 123 |
| `http_status` | 111 |
| `msg` | 108, 109, 117 |
| `ok` | 107, 112, 116 |
| `type` | 120 |

### src/gateway/gateway.cc

| 字段/键 | 源码行 |
| --- | --- |
| `command_id` | 798 |
| `device_id` | 555, 777, 778, 786, 947 |
| `id` | 779, 780, 787 |
| `reason` | 974 |
| `seq` | 775, 776, 785 |
| `timestamp` | 556, 852, 853, 857, 980, 981, 989 |
| `type` | 554, 771, 941, 943, 996, 1005 |

### src/hikvision/hikvihandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `angle` | 265, 266 |
| `angle_deg` | 263, 264, 607 |
| `azimuth` | 545, 613 |
| `command` | 217, 218, 528, 530, 586, 600 |
| `commond` | 214, 215 |
| `data` | 202, 203, 204 |
| `elevation` | 544, 612 |
| `error_message` | 553, 588, 604, 618 |
| `mode` | 225, 226, 229, 531, 585, 599 |
| `pan_speed` | 537 |
| `position_error_message` | 547 |
| `status` | 536, 552, 587, 603, 611, 617 |
| `tilt_speed` | 538 |
| `timeout_ms` | 539 |
| `type` | 526, 584, 598 |

### src/main.cc

| 字段/键 | 源码行 |
| --- | --- |
| `gloabl_map.pcd` | 41 |
| `type` | 102 |
| `value` | 102 |

### src/mapping/maphandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `address` | 158, 253 |
| `code` | 72, 96, 135, 256 |
| `device_id` | 136, 251 |
| `display_name` | 157, 252 |
| `localTime` | 73, 97, 138, 259 |
| `map_type` | 258 |
| `message` | 74, 98 |
| `status` | 137, 254 |
| `storage_path` | 255 |
| `timestamp` | 139, 260 |
| `type` | 71, 95, 134, 250 |
| `uuid` | 257 |

### src/modifyValue/modifyHandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `closed_loop_controller/box_back` | 136 |
| `closed_loop_controller/box_front` | 135 |
| `closed_loop_controller/box_half_width` | 137 |
| `closed_loop_controller/box_t_step` | 138 |
| `closed_loop_controller/brake_back_duration` | 150 |
| `closed_loop_controller/brake_back_vx` | 151 |
| `closed_loop_controller/deadzone_vx` | 142 |
| `closed_loop_controller/deadzone_vy` | 143 |
| `closed_loop_controller/deadzone_vyaw` | 144 |
| `closed_loop_controller/enable_y_velocity` | 140 |
| `closed_loop_controller/finish_dist` | 133 |
| `closed_loop_controller/heading_error_threshold` | 127 |
| `closed_loop_controller/kp_pos` | 128 |
| `closed_loop_controller/kp_yaw` | 129 |
| `closed_loop_controller/max_vx` | 130 |
| `closed_loop_controller/max_vy` | 131 |
| `closed_loop_controller/max_vyaw` | 132 |
| `closed_loop_controller/robot_max_vx` | 146 |
| `closed_loop_controller/robot_max_vy` | 147 |
| `closed_loop_controller/robot_max_vyaw` | 148 |
| `closed_loop_controller/stair_max_vx` | 153 |
| `closed_loop_controller/stair_max_vyaw` | 152 |
| `closed_loop_controller/time_forward` | 126 |
| `device_id` | 484, 506 |
| `expect_type` | 483, 505 |
| `file_type` | 473 |
| `fsm/emergency_time_` | 59 |
| `fsm/enable_rotation_check` | 64 |
| `fsm/fail_safe` | 60 |
| `fsm/is_real_world` | 62 |
| `fsm/max_replan_fail_count` | 61 |
| `fsm/navi_mode` | 55 |
| `fsm/planning_horizon` | 58 |
| `fsm/redline_crash_dist` | 70 |
| `fsm/redline_half_width` | 71 |
| `fsm/redline_step` | 72 |
| `fsm/rot_check_half_width` | 67 |
| `fsm/rot_check_len_back` | 66 |
| `fsm/rot_check_len_front` | 65 |
| `fsm/rot_check_max_sweep` | 68 |
| `fsm/thresh_no_replan` | 57 |
| `fsm/thresh_replan` | 56 |
| `grid_map/body_height` | 81 |
| `grid_map/cloud_is_world` | 52 |
| `grid_map/cx` | 85 |
| `grid_map/cy` | 86 |
| `grid_map/decay_enable` | 104 |
| `grid_map/decay_rate` | 106 |
| `grid_map/decay_time_thresh` | 105 |
| `grid_map/depth_filter_margin` | 92 |
| `grid_map/depth_filter_maxdist` | 90 |
| `grid_map/depth_filter_mindist` | 91 |
| `grid_map/double_cylinder_offset` | 80 |
| `grid_map/double_cylinder_radius` | 79 |
| `grid_map/fx` | 87 |
| `grid_map/fy` | 88 |
| `grid_map/k_depth_scaling_factor` | 93 |
| `grid_map/map_sliding_thresh` | 78 |
| `grid_map/max_ray_length` | 101 |
| `grid_map/need_extrinsic` | 53 |
| `grid_map/obstacles_inflation_z_down` | 83 |
| `grid_map/obstacles_inflation_z_up` | 82 |
| `grid_map/p_hit` | 96 |
| `grid_map/p_max` | 99 |
| `grid_map/p_min` | 98 |
| `grid_map/p_miss` | 97 |
| `grid_map/p_occ` | 100 |
| `grid_map/resolution` | 74 |
| `grid_map/sensor_type` | 51 |
| `grid_map/skip_pixel` | 94 |
| `grid_map/sliding_map_size_x` | 75 |
| `grid_map/sliding_map_size_y` | 76 |
| `grid_map/sliding_map_size_z` | 77 |
| `grid_map/vis_height` | 102 |
| `manager/control_points_distance` | 111 |
| `manager/feasibility_tolerance` | 112 |
| `manager/max_acc` | 109 |
| `manager/max_jerk` | 110 |
| `manager/max_vel` | 108 |
| `manager/planning_horizon` | 113 |
| `max_acc` | 48 |
| `max_vel` | 47 |
| `messgae` | 485, 507 |
| `name` | 470, 472, 482, 504 |
| `nav/align_angle_thresh` | 210 |
| `nav/align_done_thresh` | 211 |
| `nav/align_lookahead_dist` | 212 |
| `nav/align_stable_time` | 213 |
| `nav/arrive_angle_thresh` | 217 |
| `nav/arrive_dist_thresh` | 216 |
| `nav/default_dwell_time` | 193 |
| `nav/enable_start_align` | 209 |
| `nav/external_timeout` | 201 |
| `nav/max_vx` | 203 |
| `nav/max_vy` | 204 |
| `nav/max_w` | 205 |
| `nav/precision_dist_thresh` | 215 |
| `nav/rdp_epsilon` | 219 |
| `nav/robot_center_offset` | 200 |
| `nav/sim_initial_pose/x` | 195 |
| `nav/sim_initial_pose/y` | 196 |
| `nav/sim_initial_pose/yaw` | 198 |
| `nav/sim_initial_pose/z` | 197 |
| `nav/slope_thresh_deg` | 222 |
| `nav/stair_window_size` | 221 |
| `nav/step_z_thresh` | 220 |
| `nav/switch_distance` | 207 |
| `nav/timeout` | 192 |
| `optimization/acc_tolerance` | 124 |
| `optimization/dist0` | 119 |
| `optimization/enable_narrow_adaptation` | 120 |
| `optimization/lambda_collision` | 116 |
| `optimization/lambda_feasibility` | 117 |
| `optimization/lambda_fitness` | 118 |
| `optimization/lambda_smooth` | 115 |
| `optimization/max_acc` | 123 |
| `optimization/max_vel` | 121 |
| `optimization/vel_tolerance` | 122 |
| `pct_planner/max_heading_rate` | 174 |
| `pct_planner/use_quintic` | 173 |
| `planning_horizon` | 49 |
| `ros/base_frame` | 166 |
| `ros/dock_topic` | 163 |
| `ros/goal_topic` | 161 |
| `ros/layer_C_topic` | 170 |
| `ros/layer_G_topic` | 169 |
| `ros/map_frame` | 165 |
| `ros/odom_topic` | 160 |
| `ros/path_topic` | 162 |
| `ros/pointcloud_topic` | 168 |
| `ros/tomogram_topic` | 171 |
| `scene` | 158 |
| `status` | 481, 503 |
| `tomography/cost_barrier` | 188 |
| `tomography/ground_h` | 178 |
| `tomography/inflation` | 190 |
| `tomography/interval_free` | 182 |
| `tomography/interval_min` | 181 |
| `tomography/kernel_size` | 180 |
| `tomography/resolution` | 176 |
| `tomography/safe_margin` | 189 |
| `tomography/slice_dh` | 177 |
| `tomography/slope_max` | 184 |
| `tomography/standable_ratio` | 187 |
| `tomography/step_max` | 185 |
| `type` | 480, 502 |
| `value` | 470, 474 |

### src/motionControl/motionHandle.cc

| 字段/键 | 源码行 |
| --- | --- |
| `angular` | 31 |
| `angular_velocity` | 138 |
| `command` | 107 |
| `data` | 95 |
| `device_id` | 97 |
| `linear` | 30 |
| `linear_velocity` | 137 |
| `parameters` | 109, 110 |
| `robot_id` | 97 |
| `source` | 26 |
| `target_mode` | 27 |
| `timestamp` | 99 |
| `ttl_sec` | 28 |
| `twist` | 29 |
| `type` | 96 |
| `x` | 30, 31 |
| `y` | 30, 31 |
| `z` | 30, 31 |

### src/robotState/robotState.cc

| 字段/键 | 源码行 |
| --- | --- |
| `active_alarm_levels` | 97 |
| `active_flags` | 98 |
| `address` | 160 |
| `bms_info` | 217 |
| `device_id` | 204 |
| `display_name` | 161 |
| `has_bms_info` | 207 |
| `has_robot_state` | 206 |
| `key` | 28, 40 |
| `label` | 29, 41 |
| `level` | 30 |
| `map` | 209 |
| `navigation_env_active` | 208 |
| `robot_state` | 214 |
| `soc_percent` | 96 |
| `timestamp` | 203 |
| `type` | 202 |
| `updated_source` | 205 |
| `version` | 210 |
| `voltage_v` | 95 |

## C++ 方法与函数定位

文本扫描类作用域定义和头文件中声明形式；多行参数取其起始行。此处是检索辅助，不宣称是 AST 解析的完整可调用符号表，保留了声明但未实现的入口。

### include/CommandExecutor.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `CommandExecutor` | 47 | `CommandExecutor() = default;` |
| `CommandExecutor` | 48 | `CommandExecutor(ros::NodeHandle& nh,` |
| `~CommandExecutor` | 54 | `~CommandExecutor();` |
| `ros_prepare` | 56 | `void ros_prepare(const json& payload_json);` |
| `execute` | 57 | `void execute(const json& payload_json);` |
| `robot_bt` | 58 | `void robot_bt(std::string cmd_type, json data);` |
| `send_to_cloud` | 59 | `void send_to_cloud(const std::string& msg);` |
| `try_flush_msgs` | 60 | `void try_flush_msgs();` |
| `setGateway` | 61 | `void setGateway(gateway* gw);` |
| `handle_initial_pose` | 63 | `void handle_initial_pose(const json& data);` |
| `handle_start_navigation` | 64 | `void handle_start_navigation(const json& data);` |
| `handle_navigation_task` | 65 | `void handle_navigation_task(const json& data);` |
| `handle_set_dock_pose` | 66 | `void handle_set_dock_pose(const json& data);` |
| `handle_stop_navigation` | 67 | `bool handle_stop_navigation(const json& data);` |
| `handle_start_mapping` | 68 | `void handle_start_mapping(const json& data);` |
| `handle_stop_and_uploadmap` | 69 | `void handle_stop_and_uploadmap(const json& data);` |
| `handle_start_sh` | 70 | `void handle_start_sh(const json& data);` |
| `start_navigation_async` | 71 | `void start_navigation_async(json data, std::string uuid_old, std::string uuid_new,` |
| `stop_and_uploadmap_async` | 73 | `void stop_and_uploadmap_async(json data);` |
| `get_last_storage_path` | 74 | `std::string get_last_storage_path() const;` |
| `set_upload_enable` | 75 | `void set_upload_enable(bool enable);` |
| `get_localization_ready` | 76 | `bool get_localization_ready() const;` |
| `get_current_map_id` | 77 | `std::string get_current_map_id() const;` |
| `set_current_map_id` | 78 | `void set_current_map_id(const std::string& id);` |
| `get_state_name` | 79 | `std::string get_state_name() const;` |
| `loc_callback` | 81 | `void loc_callback(const std_msgs::Int8::ConstPtr& msg);` |
| `get_robot_position` | 82 | `bool get_robot_position(double& x, double& y, double& z);` |
| `get_robot_orientation` | 83 | `bool get_robot_orientation(double& roll, double& pitch, double& yaw);` |
| `exec_upload_cmd` | 84 | `void exec_upload_cmd(const json& data);` |
| `start_add_task` | 86 | `void start_add_task();` |
| `start_add_point` | 87 | `void start_add_point();` |
| `stop_add_point` | 88 | `void stop_add_point();` |
| `stop_add_task` | 89 | `void stop_add_task();` |
| `exec_rosservice_cmd` | 90 | `void exec_rosservice_cmd(const std::string& type, const std::string& service_name);` |
| `add_upload_sn` | 92 | `void add_upload_sn(std::string sn, std::string http_ip, int http_port) {` |
| `std::to_string` | 94 | `std::string url = "http://" + http_ip + ":" + std::to_string(http_port) + "/sn_add?sn=" + sn;` |
| `curl_easy_setopt` | 102 | `curl_easy_setopt(curl, CURLOPT_URL, url.c_str());` |
| `curl_easy_setopt` | 104 | `curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,` |
| `curl_easy_setopt` | 111 | `curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);` |
| `curl_easy_cleanup` | 122 | `curl_easy_cleanup(curl);` |
| `admit_behavior_command` | 126 | `bool admit_behavior_command(const std::string& type);` |
| `handle_follow_command` | 127 | `void handle_follow_command(const std::string& type, const json& data);` |
| `publish_fsm_state` | 128 | `void publish_fsm_state(const std::string& command, const std::string& status,` |
| `invalidate_navigation_requests` | 130 | `void invalidate_navigation_requests();` |
| `init_ros_publishers` | 133 | `void init_ros_publishers();` |
| `init_ros_subscribers` | 134 | `void init_ros_subscribers();` |
| `global_path_callback` | 135 | `void global_path_callback(const nav_msgs::Path::ConstPtr& msg);` |
| `localization_callback` | 136 | `void localization_callback(const nav_msgs::Odometry::ConstPtr& msg);` |
| `initial_pose_rosout_callback` | 137 | `void initial_pose_rosout_callback(const rosgraph_msgs::Log::ConstPtr& msg);` |
| `complete_initial_pose_result` | 138 | `void complete_initial_pose_result(unsigned long long attempt_id, bool success, const std::string& message);` |
| `complete_navigation_env_ready` | 139 | `void complete_navigation_env_ready(unsigned long long attempt_id, bool success, const std::string& message);` |
| `complete_navigation_task_result` | 140 | `void complete_navigation_task_result(unsigned long long attempt_id, bool success, const std::string& message);` |
| `complete_clear_waypoints_result` | 141 | `void complete_clear_waypoints_result(unsigned long long attempt_id, bool success, const std::string& message);` |
| `handle_navigation_planning_rosout` | 142 | `void handle_navigation_planning_rosout(const std::string& message);` |
| `send_navigation_planning_failure` | 143 | `void send_navigation_planning_failure();` |
| `try_begin_command` | 144 | `bool try_begin_command(const std::string& cmd_type);` |
| `finish_command` | 145 | `void finish_command(const std::string& cmd_type);` |
| `is_managed_command` | 146 | `bool is_managed_command(const std::string& cmd_type) const;` |
| `enqueue_task_state_request` | 147 | `void enqueue_task_state_request(const std::string& request_type, bool target_paused);` |
| `process_next_task_state_request` | 148 | `void process_next_task_state_request();` |
| `complete_task_state_transition` | 149 | `void complete_task_state_transition(bool success, int ret);` |
| `telemetry_loop` | 150 | `void telemetry_loop();` |
| `handle_set_position_upload_frequency` | 151 | `void handle_set_position_upload_frequency(const json& data);` |
| `should_send_upload_position` | 152 | `bool should_send_upload_position();` |
| `is_pid_still_running` | 153 | `bool is_pid_still_running(pid_t pid) const;` |
| `is_navigation_environment_running` | 154 | `bool is_navigation_environment_running(std::string& reason);` |
| `launch_background_task` | 155 | `void launch_background_task(std::thread&& worker);` |
| `log_runtime_flags` | 156 | `void log_runtime_flags(const char* stage) const;` |
| `log_wait_status` | 157 | `void log_wait_status(const std::string& name, int status) const;` |
| `log_file_access` | 158 | `bool log_file_access(const std::string& name, const std::string& path, int mode) const;` |
| `shell_quote` | 159 | `std::string shell_quote(const std::string& value) const;` |
| `script_log_command` | 160 | `std::string script_log_command(const std::string& script_path, const std::string& tag) const;` |

### include/RobotFsm.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `snapshot` | 10 | `RobotDto snapshot() const {` |
| `lock` | 11 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `begin_navigation` | 15 | `bool begin_navigation() {` |
| `lock` | 16 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `navigation_current` | 26 | `bool navigation_current(unsigned long long generation) const {` |
| `lock` | 27 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `navigation_ready` | 31 | `void navigation_ready(bool success, const std::string& reason) {` |
| `lock` | 32 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `begin_follow` | 39 | `FollowDecision begin_follow() {` |
| `lock` | 40 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `begin_stop` | 53 | `bool begin_stop() {` |
| `lock` | 54 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `stopped` | 64 | `void stopped(bool success, const std::string& reason, bool follow = false) {` |
| `lock` | 65 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `follow_started` | 74 | `void follow_started() {` |
| `lock` | 75 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `stop_follow` | 82 | `bool stop_follow() {` |
| `lock` | 83 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `task_state` | 92 | `void task_state(TaskStatus status) {` |
| `lock` | 93 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `localization_ready` | 100 | `void localization_ready(bool ready) {` |
| `lock` | 101 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `mapping` | 105 | `void mapping(bool active) {` |
| `lock` | 106 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `begin_mapping` | 110 | `bool begin_mapping() {` |
| `lock` | 111 | `std::lock_guard<std::mutex> lock(mutex_);` |
| `is_navigation` | 119 | `static bool is_navigation(MotionMode mode) {` |

### include/common/cmdline.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `Copyright` | 2 | `Copyright (c) 2009, Hideyuki Tanaka` |
| `cast` | 49 | `static Target cast(const Source &arg){` |
| `std::bad_cast` | 53 | `throw std::bad_cast();` |
| `cast` | 62 | `static Target cast(const Source &arg){` |
| `cast` | 70 | `static std::string cast(const Source &arg){` |
| `cast` | 80 | `static Target cast(const std::string &arg){` |
| `ss` | 82 | `std::istringstream ss(arg);` |
| `std::bad_cast` | 84 | `throw std::bad_cast();` |
| `lexical_cast` | 100 | `Target lexical_cast(const Source &arg)` |
| `demangle` | 105 | `static inline std::string demangle(const std::string &name)` |
| `abi::__cxa_demangle` | 108 | `char *p=abi::__cxa_demangle(name.c_str(), 0, 0, &status);` |
| `ret` | 109 | `std::string ret(p);` |
| `free` | 110 | `free(p);` |
| `readable_typename` | 115 | `std::string readable_typename()` |
| `demangle` | 117 | `return demangle(typeid(T).name());` |
| `default_value` | 121 | `std::string default_value(T def)` |
| `cmdline_error` | 138 | `cmdline_error(const std::string &msg): msg(msg){}` |
| `~cmdline_error` | 139 | `~cmdline_error() throw() {}` |
| `operator` | 147 | `T operator()(const std::string &str){` |
| `range_reader` | 154 | `range_reader(const T &low, const T &high): low(low), high(high) {}` |
| `operator` | 155 | `T operator()(const std::string &s) const {` |
| `cmdline::cmdline_error` | 157 | `if (!(ret>=low && ret<=high)) throw cmdline::cmdline_error("range_error");` |
| `operator` | 172 | `T operator()(const std::string &s){` |
| `std::find` | 174 | `if (std::find(alt.begin(), alt.end(), ret)==alt.end())` |
| `cmdline_error` | 175 | `throw cmdline_error("");` |
| `parser` | 312 | `parser(){` |
| `~parser` | 314 | `~parser(){` |
| `add` | 320 | `void add(const std::string &name,` |
| `add` | 329 | `void add(const std::string &name,` |
| `add` | 334 | `add(name, short_name, desc, need, def, default_reader<T>());` |
| `add` | 338 | `void add(const std::string &name,` |
| `footer` | 349 | `void footer(const std::string &f){` |
| `set_program_name` | 353 | `void set_program_name(const std::string &name){` |
| `exist` | 357 | `bool exist(const std::string &name) const {` |
| `parse` | 374 | `bool parse(const std::string &arg){` |
| `parse` | 413 | `return parse(args);` |
| `parse` | 416 | `bool parse(const std::vector<std::string> &args){` |
| `argv` | 418 | `std::vector<const char*> argv(argc);` |
| `parse` | 423 | `return parse(argc, &argv[0]);` |
| `parse` | 426 | `bool parse(int argc, const char * const argv[]){` |
| `std::string` | 445 | `errors.push_back(std::string("short option '")+initial+"' is ambiguous");` |
| `name` | 456 | `std::string name(argv[i]+2, p);` |
| `val` | 457 | `std::string val(p+1);` |
| `set_option` | 458 | `set_option(name, val);` |
| `name` | 461 | `std::string name(argv[i]+2);` |
| `set_option` | 473 | `set_option(name, argv[i]);` |
| `set_option` | 477 | `set_option(name);` |
| `std::string` | 487 | `errors.push_back(std::string("undefined short option: -")+argv[i][j-1]);` |
| `std::string` | 491 | `errors.push_back(std::string("ambiguous short option: -")+argv[i][j-1]);` |
| `set_option` | 494 | `set_option(lookup[argv[i][j-1]]);` |
| `std::string` | 498 | `errors.push_back(std::string("undefined short option: -")+last);` |
| `std::string` | 502 | `errors.push_back(std::string("ambiguous short option: -")+last);` |
| `set_option` | 507 | `set_option(lookup[last], argv[i+1]);` |
| `set_option` | 511 | `set_option(lookup[last]);` |
| `std::string` | 522 | `errors.push_back("need option: --"+std::string(p->first));` |
| `parse_check` | 527 | `void parse_check(const std::string &arg){` |
| `add` | 529 | `add("help", '?', "print this message");` |
| `check` | 530 | `check(0, parse(arg));` |
| `parse_check` | 533 | `void parse_check(const std::vector<std::string> &args){` |
| `add` | 535 | `add("help", '?', "print this message");` |
| `check` | 536 | `check(args.size(), parse(args));` |
| `parse_check` | 539 | `void parse_check(int argc, char *argv[]){` |
| `add` | 541 | `add("help", '?', "print this message");` |
| `check` | 542 | `check(argc, parse(argc, argv));` |
| `error` | 545 | `std::string error() const{` |
| `error_full` | 549 | `std::string error_full() const{` |
| `usage` | 556 | `std::string usage() const {` |
| `std::max` | 569 | `max_width=std::max(max_width, ordered[i]->name().length());` |
| `check` | 589 | `void check(int argc, bool ok){` |
| `exit` | 592 | `exit(0);` |
| `exit` | 597 | `exit(1);` |
| `set_option` | 601 | `void set_option(const std::string &name){` |
| `set_option` | 612 | `void set_option(const std::string &name, const std::string &value){` |
| `~option_base` | 625 | `virtual ~option_base(){}` |
| `has_value` | 627 | `virtual bool has_value() const=0;` |
| `set` | 628 | `virtual bool set()=0;` |
| `set` | 629 | `virtual bool set(const std::string &value)=0;` |
| `has_set` | 630 | `virtual bool has_set() const=0;` |
| `valid` | 631 | `virtual bool valid() const=0;` |
| `must` | 632 | `virtual bool must() const=0;` |
| `short_name` | 635 | `virtual char short_name() const=0;` |
| `short_description` | 637 | `virtual std::string short_description() const=0;` |
| `option_without_value` | 642 | `option_without_value(const std::string &name,` |
| `~option_without_value` | 647 | `~option_without_value(){}` |
| `set` | 651 | `bool set(){` |
| `set` | 656 | `bool set(const std::string &){` |
| `has_set` | 660 | `bool has_set() const {` |
| `valid` | 664 | `bool valid() const{` |
| `must` | 668 | `bool must() const{` |
| `short_name` | 676 | `char short_name() const{` |
| `short_description` | 684 | `std::string short_description() const{` |
| `option_with_value` | 698 | `option_with_value(const std::string &name,` |
| `~option_with_value` | 707 | `~option_with_value(){}` |
| `set` | 715 | `bool set(){` |
| `set` | 719 | `bool set(const std::string &value){` |
| `has_set` | 730 | `bool has_set() const{` |
| `valid` | 734 | `bool valid() const{` |
| `must` | 739 | `bool must() const{` |
| `short_name` | 747 | `char short_name() const{` |
| `short_description` | 755 | `std::string short_description() const{` |
| `full_description` | 760 | `std::string full_description(const std::string &desc){` |
| `read` | 767 | `virtual T read(const std::string &s)=0;` |
| `option_with_value_with_reader` | 782 | `option_with_value_with_reader(const std::string &name,` |
| `read` | 792 | `T read(const std::string &s){` |
| `reader` | 793 | `return reader(s);` |

### include/common/log_manager.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `get_logger` | 23 | `std::shared_ptr<spdlog::logger> get_logger() {` |
| `get_log_file_path` | 27 | `std::string get_log_file_path() const {` |
| `LogManager` | 32 | `LogManager() {` |
| `init_log_dir` | 33 | `init_log_dir();` |
| `cleanup_old_logs` | 34 | `cleanup_old_logs();` |
| `init_logger` | 35 | `init_logger();` |
| `~LogManager` | 38 | `~LogManager() {` |
| `spdlog::shutdown` | 39 | `spdlog::shutdown();` |
| `init_log_dir` | 49 | `void init_log_dir() {` |
| `fs::exists` | 51 | `if (!fs::exists(log_dir_)) {` |
| `fs::create_directories` | 52 | `fs::create_directories(log_dir_);` |
| `get_log_filename` | 59 | `std::string get_log_filename() {` |
| `system_clock::now` | 60 | `auto now = std::chrono::system_clock::now();` |
| `system_clock::to_time_t` | 61 | `std::time_t t = std::chrono::system_clock::to_time_t(now);` |
| `localtime_r` | 64 | `localtime_r(&t, &tm);` |
| `std::strftime` | 67 | `std::strftime(buffer, sizeof(buffer), "robot_%Y_%m_%d.log", &tm);` |
| `init_logger` | 72 | `void init_logger() {` |
| `spdlog::basic_logger_mt` | 78 | `logger_ = spdlog::basic_logger_mt("robot_logger", filename);` |
| `spdlog::set_default_logger` | 85 | `spdlog::set_default_logger(logger_);` |
| `cleanup_old_logs` | 92 | `void cleanup_old_logs() {` |
| `fs::directory_iterator` | 96 | `for (auto& p : fs::directory_iterator(log_dir_)) {` |
| `std::sort` | 103 | `std::sort(log_files.begin(), log_files.end(),` |
| `fs::last_write_time` | 105 | `return fs::last_write_time(a) < fs::last_write_time(b);` |
| `fs::remove` | 112 | `fs::remove(log_files[i]);` |
| `info` | 124 | `inline void info(Args&&... args) {` |
| `LogManager::instance` | 125 | `auto logger = LogManager::instance().get_logger();` |
| `warn` | 132 | `inline void warn(Args&&... args) {` |
| `LogManager::instance` | 133 | `auto logger = LogManager::instance().get_logger();` |
| `error` | 140 | `inline void error(Args&&... args) {` |
| `LogManager::instance` | 141 | `auto logger = LogManager::instance().get_logger();` |
| `debug` | 148 | `inline void debug(Args&&... args) {` |
| `LogManager::instance` | 149 | `auto logger = LogManager::instance().get_logger();` |
| `robot_log::info` | 156 | `#define LOG_INFO(...)  robot_log::info(__VA_ARGS__)` |
| `robot_log::warn` | 157 | `#define LOG_WARN(...)  robot_log::warn(__VA_ARGS__)` |
| `robot_log::error` | 158 | `#define LOG_ERROR(...) robot_log::error(__VA_ARGS__)` |
| `robot_log::debug` | 159 | `#define LOG_DEBUG(...) robot_log::debug(__VA_ARGS__)` |

### include/common/utils.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `url_encode` | 17 | `inline std::string url_encode(const std::string& value) {` |
| `std::isalnum` | 23 | `if (std::isalnum(c) \|\| c == '-' \|\| c == '_' \|\| c == '.' \|\| c == '~') {` |
| `std::setw` | 28 | `escaped << '%' << std::setw(2) << static_cast<int>(c);` |
| `exec_cmd` | 35 | `inline std::string exec_cmd(const std::string& cmd)` |
| `ROS_WARN` | 49 | `ROS_WARN("exec_cmd returned non-zero status: %d, cmd=%s", status, cmd.c_str());` |
| `shell_single_quote` | 54 | `inline static std::string shell_single_quote(const std::string& value)` |
| `trim_copy` | 68 | `inline static std::string trim_copy(const std::string& value)` |
| `text_after_marker` | 78 | `inline static std::string text_after_marker(const std::string& value, const std::string& marker)` |
| `trim_copy` | 84 | `return trim_copy(value.substr(pos + marker.size()));` |
| `get_uuid` | 87 | `inline std::string get_uuid(const std::string& address, const std::string& display_name,` |
| `std::to_string` | 98 | `"curl -sS -m 10 -X POST http://" + http_ip + ":" + std::to_string(http_port) + "/get_uuid "` |
| `ROS_ERROR` | 104 | `ROS_ERROR("get_uuid failed: empty response, address=%s, display_name=%s",` |
| `json::parse` | 110 | `auto j = json::parse(response);` |
| `ROS_ERROR` | 112 | `ROS_ERROR("get_uuid failed: %s, response=%s",` |
| `ROS_ERROR` | 118 | `ROS_ERROR("get_uuid failed: missing uuid, response=%s", response.c_str());` |
| `ROS_ERROR` | 122 | `ROS_ERROR("get_uuid parse failed: %s, raw response=%s", e.what(), response.c_str());` |
| `unix_timestamp_ms` | 127 | `inline static long long unix_timestamp_ms()` |
| `system_clock::now` | 130 | `//     std::chrono::system_clock::now().time_since_epoch()).count();` |
| `system_clock::to_time_t` | 132 | `return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());` |
| `parse_completed_task_round` | 135 | `inline static bool parse_completed_task_round(const std::string& message, int& round)` |
| `std::string` | 143 | `size_t digits_begin = marker_pos + std::string("第").size();` |
| `std::isspace` | 145 | `std::isspace(static_cast<unsigned char>(message[digits_begin]))) {` |
| `std::isdigit` | 151 | `std::isdigit(static_cast<unsigned char>(message[digits_end]))) {` |
| `std::isspace` | 160 | `std::isspace(static_cast<unsigned char>(message[suffix_pos]))) {` |
| `std::strtol` | 170 | `const long value = std::strtol(digits.c_str(), &end, 10);` |
| `getTimeNow` | 180 | `inline std::string getTimeNow() {` |
| `std::time` | 181 | `std::time_t now = std::time(nullptr);` |
| `std::localtime` | 182 | `std::tm local_time = *std::localtime(&now);` |
| `std::strftime` | 185 | `std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);` |
| `get_serial_number` | 190 | `inline std::string get_serial_number() {` |
| `pclose` | 210 | `pclose(pipe);` |
| `trim_copy` | 211 | `return trim_copy(result);` |

### include/config/yamlConfig.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `loadConfig` | 36 | `Config loadConfig(const std::string& path) {` |
| `YAML::LoadFile` | 37 | `YAML::Node yaml = YAML::LoadFile(path);` |
| `std::runtime_error` | 75 | `throw std::runtime_error("light.low_battery_percent must be between 0 and 100");` |

### include/http_map_uploader.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `HttpMapUploader` | 31 | `HttpMapUploader(const std::string& host, const int& port)` |
| `ensure_curl_global_init` | 33 | `ensure_curl_global_init();` |
| `set_progress_callback` | 36 | `void set_progress_callback(ProgressCallback cb) {` |
| `std::move` | 37 | `progress_callback_ = std::move(cb);` |
| `connect` | 40 | `bool connect() {` |
| `disconnect` | 50 | `void disconnect() {` |
| `upload_files_parallel` | 54 | `bool upload_files_parallel(const std::string& local_dir,` |
| `steady_clock::now` | 63 | `auto start = std::chrono::steady_clock::now();` |
| `fs::path` | 68 | `fs::path local = fs::path(local_dir) / file;` |
| `fs::exists` | 69 | `if (!fs::exists(local)) {` |
| `fs::file_size` | 74 | `const size_t file_size = fs::file_size(local);` |
| `std::min` | 83 | `const size_t worker_count = std::min(kMaxParallelUploads, jobs.size());` |
| `steady_clock::now` | 118 | `std::chrono::steady_clock::now() - start).count();` |
| `upload_files` | 130 | `bool upload_files(const std::string& local_dir,` |
| `steady_clock::now` | 133 | `auto start = std::chrono::steady_clock::now();` |
| `fs::path` | 138 | `fs::path local = fs::path(local_dir) / filename;` |
| `fs::exists` | 139 | `if (!fs::exists(local)) {` |
| `fs::file_size` | 144 | `const size_t file_size = fs::file_size(local);` |
| `steady_clock::now` | 154 | `std::chrono::steady_clock::now() - start).count();` |
| `download_files` | 162 | `bool download_files(const std::string& remote_dir,` |
| `fs::create_directories` | 165 | `fs::create_directories(local_dir);` |
| `steady_clock::now` | 170 | `auto start = std::chrono::steady_clock::now();` |
| `fs::path` | 173 | `fs::path local = fs::path(local_dir) / file;` |
| `steady_clock::now` | 180 | `std::chrono::steady_clock::now() - start).count();` |
| `operator` | 195 | `void operator()(CURL* curl) const {` |
| `curl_easy_cleanup` | 197 | `curl_easy_cleanup(curl);` |
| `operator` | 203 | `void operator()(curl_slist* list) const {` |
| `curl_slist_free_all` | 205 | `curl_slist_free_all(list);` |
| `ensure_curl_global_init` | 234 | `static void ensure_curl_global_init() {` |
| `base_url` | 239 | `std::string base_url() const {` |
| `std::to_string` | 240 | `return "http://" + host_ + ":" + std::to_string(port_);` |
| `make_url` | 243 | `std::string make_url(const std::string& path) const {` |
| `base_url` | 244 | `return base_url() + path;` |
| `disable_proxy` | 247 | `void disable_proxy(CURL* curl) const {` |
| `curl_easy_setopt` | 249 | `curl_easy_setopt(curl, CURLOPT_PROXY, "");` |
| `curl_easy_setopt` | 250 | `curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");` |
| `perform_simple_request` | 253 | `RequestResult perform_simple_request(const std::string& method,` |
| `curl` | 258 | `std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());` |
| `curl_easy_setopt` | 266 | `curl_easy_setopt(curl.get(), CURLOPT_URL, make_url(path).c_str());` |
| `disable_proxy` | 267 | `disable_proxy(curl.get());` |
| `curl_easy_setopt` | 268 | `curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);` |
| `curl_easy_setopt` | 269 | `curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);` |
| `curl_easy_setopt` | 270 | `curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);` |
| `curl_easy_setopt` | 271 | `curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);` |
| `curl_easy_setopt` | 272 | `curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);` |
| `curl_easy_setopt` | 273 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_string_callback);` |
| `curl_easy_setopt` | 274 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);` |
| `curl_easy_setopt` | 277 | `curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);` |
| `curl_easy_setopt` | 279 | `curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);` |
| `curl_easy_setopt` | 281 | `curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body->c_str());` |
| `curl_easy_setopt` | 282 | `curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,` |
| `curl_easy_setopt` | 294 | `curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, list.get());` |
| `curl_easy_getinfo` | 303 | `curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &result.http_code);` |
| `std::move` | 304 | `result.body = std::move(response_body);` |
| `write_string_callback` | 309 | `static size_t write_string_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {` |
| `read_file_callback` | 316 | `static size_t read_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {` |
| `write_file_callback` | 327 | `static size_t write_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {` |
| `xfer_progress_callback` | 350 | `static int xfer_progress_callback(void* clientp,` |
| `download_progress_callback` | 369 | `static int download_progress_callback(void* clientp,` |
| `upload_one_threadsafe` | 388 | `bool upload_one_threadsafe(const std::string& local,` |
| `chrono::seconds` | 394 | `const auto RETRY_DELAY = std::chrono::seconds(2);` |
| `fs::exists` | 396 | `if (!fs::exists(local)) {` |
| `steady_clock::now` | 401 | `auto start = std::chrono::steady_clock::now();` |
| `std::strerror` | 418 | `<< ", reason: " << std::strerror(errno)` |
| `fs::exists` | 419 | `<< ", exists: " << fs::exists(local) << "\n";` |
| `curl` | 423 | `std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());` |
| `curl_easy_setopt` | 437 | `curl_easy_setopt(curl.get(), CURLOPT_URL, full_url.c_str());` |
| `disable_proxy` | 438 | `disable_proxy(curl.get());` |
| `curl_easy_setopt` | 439 | `curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);` |
| `curl_easy_setopt` | 440 | `curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, nullptr);` |
| `curl_easy_setopt` | 441 | `curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,` |
| `curl_easy_setopt` | 443 | `curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, &HttpMapUploader::read_file_callback);` |
| `curl_easy_setopt` | 444 | `curl_easy_setopt(curl.get(), CURLOPT_READDATA, &ctx);` |
| `curl_easy_setopt` | 445 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_string_callback);` |
| `curl_easy_setopt` | 446 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);` |
| `curl_easy_setopt` | 447 | `curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());` |
| `curl_easy_setopt` | 448 | `curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);` |
| `curl_easy_setopt` | 449 | `curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 1800L);` |
| `curl_easy_setopt` | 450 | `curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);` |
| `curl_easy_setopt` | 451 | `curl_easy_setopt(curl.get(), CURLOPT_TCP_NODELAY, 1L);` |
| `curl_easy_setopt` | 452 | `curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);` |
| `curl_easy_setopt` | 453 | `curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);` |
| `curl_easy_setopt` | 454 | `curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);` |
| `curl_easy_setopt` | 455 | `curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &HttpMapUploader::xfer_progress_callback);` |
| `curl_easy_setopt` | 456 | `curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &ctx);` |
| `curl_easy_getinfo` | 460 | `curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);` |
| `print_progress` | 463 | `print_progress(file_size, file_size, filename);` |
| `steady_clock::now` | 466 | `std::chrono::steady_clock::now() - start).count();` |
| `this_thread::sleep_for` | 484 | `std::this_thread::sleep_for(RETRY_DELAY);` |
| `url_encode_path` | 492 | `std::string url_encode_path(const std::string& path) {` |
| `download_one_threadsafe` | 503 | `bool download_one_threadsafe(const std::string& remote_dir,` |
| `curl` | 507 | `std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());` |
| `fs::create_directories` | 513 | `fs::create_directories(fs::path(local).parent_path());` |
| `curl_easy_setopt` | 531 | `curl_easy_setopt(curl.get(), CURLOPT_URL, full_url.c_str());` |
| `disable_proxy` | 532 | `disable_proxy(curl.get());` |
| `curl_easy_setopt` | 533 | `curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);` |
| `curl_easy_setopt` | 534 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_file_callback);` |
| `curl_easy_setopt` | 535 | `curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &ctx);` |
| `curl_easy_setopt` | 536 | `curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);` |
| `curl_easy_setopt` | 537 | `curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 1800L);` |
| `curl_easy_setopt` | 538 | `curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);` |
| `curl_easy_setopt` | 539 | `curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);` |
| `curl_easy_setopt` | 540 | `curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);` |
| `curl_easy_setopt` | 541 | `curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);` |
| `curl_easy_setopt` | 542 | `curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &HttpMapUploader::download_progress_callback);` |
| `curl_easy_setopt` | 543 | `curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &ctx);` |
| `steady_clock::now` | 545 | `auto start = std::chrono::steady_clock::now();` |
| `curl_easy_getinfo` | 548 | `curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);` |
| `curl_easy_getinfo` | 550 | `curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &content_length);` |
| `print_progress` | 555 | `print_progress(ctx.received, ctx.file_size, filename);` |
| `steady_clock::now` | 559 | `std::chrono::steady_clock::now() - start).count();` |
| `print_progress` | 577 | `void print_progress(size_t current, size_t total, const std::string& filename = "") {` |
| `steady_clock::now` | 586 | `auto now = std::chrono::steady_clock::now();` |
| `lock` | 588 | `std::lock_guard<std::mutex> lock(io_mutex_);` |
| `std::setprecision` | 605 | `<< std::setprecision(1)` |
| `progress_callback_` | 620 | `progress_callback_(prog_msg.dump());` |

### include/manager.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `MapManager` | 22 | `MapManager(const std::string& http_ip, const int& http_port)` |
| `set_progress_callback` | 26 | `void set_progress_callback(ProgressCallback cb) {` |
| `std::move` | 27 | `progress_callback_ = std::move(cb);` |
| `upload_map` | 31 | `std::pair<std::string, std::string> upload_map(const std::string& device_id, const std::string& address,` |
| `uploader` | 63 | `HttpMapUploader uploader(http_ip_, api_port_);` |
| `upload_map_sh` | 78 | `std::pair<std::string, std::string> upload_map_sh(const std::string& device_id, const std::string& address,` |
| `uploader` | 85 | `HttpMapUploader uploader(http_ip_, api_port_);` |
| `download_and_deploy` | 103 | `bool download_and_deploy(const std::string& address, const std::string& display_name,` |
| `uploader` | 116 | `HttpMapUploader uploader(http_ip_, api_port_);` |
| `return_http_url` | 132 | `std::string return_http_url(const std::string& address, const std::string& display_name,` |
| `http_post` | 148 | `bool http_post(const std::string& path, const json& payload, json& result) {` |
| `lock` | 149 | `std::lock_guard<std::mutex> lock(http_mutex_);` |
| `json::parse` | 160 | `result = json::parse(res->body);` |
| `http_get` | 164 | `bool http_get(const std::string& path, const std::map<std::string, std::string>& params, json& result) {` |
| `lock` | 165 | `std::lock_guard<std::mutex> lock(http_mutex_);` |
| `json::parse` | 182 | `result = json::parse(res->body);` |
| `deploy` | 186 | `bool deploy(const std::string& src, const std::string& dst) {` |
| `fs::exists` | 188 | `if(fs::exists(dst)) {` |
| `fs::remove_all` | 189 | `fs::remove_all(dst);` |
| `fs::create_directories` | 191 | `fs::create_directories(dst);` |
| `fs::recursive_directory_iterator` | 193 | `for(auto& p : fs::recursive_directory_iterator(src)) {` |
| `fs::relative` | 194 | `fs::path relative = fs::relative(p.path(), src);` |
| `fs::path` | 195 | `fs::path target = fs::path(dst) / relative;` |
| `fs::create_directories` | 197 | `fs::create_directories(target);` |
| `fs::copy` | 199 | `fs::copy(p.path(), target, fs::copy_options::overwrite_existing);` |

### src/CommandExecutor.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `CommandExecutor::setGateway` | 26 | `void CommandExecutor::setGateway(gateway* gw) {` |
| `CommandExecutor::CommandExecutor` | 36 | `CommandExecutor::CommandExecutor(ros::NodeHandle& nh,` |
| `CommandExecutor::~CommandExecutor` | 154 | `CommandExecutor::~CommandExecutor() {` |
| `std::move` | 166 | `workers = std::move(background_workers_);` |
| `CommandExecutor::init_ros_publishers` | 186 | `void CommandExecutor::init_ros_publishers() {` |
| `CommandExecutor::init_ros_subscribers` | 198 | `void CommandExecutor::init_ros_subscribers() {` |
| `CommandExecutor::send_to_cloud` | 217 | `void CommandExecutor::send_to_cloud(const std::string& msg) {` |
| `std::remove_if` | 227 | `std::remove_if(` |
| `CommandExecutor::should_send_upload_position` | 260 | `bool CommandExecutor::should_send_upload_position() {` |
| `system_clock::now` | 262 | `std::chrono::system_clock::now().time_since_epoch()).count();` |
| `CommandExecutor::handle_set_position_upload_frequency` | 280 | `void CommandExecutor::handle_set_position_upload_frequency(const json& data) {` |
| `std::stod` | 297 | `frequency_hz = std::stod(text, &parsed_chars);` |
| `std::isfinite` | 305 | `valid = valid && std::isfinite(frequency_hz) && frequency_hz > 0.0 &&` |
| `std::llround` | 320 | `1, static_cast<long long>(std::llround(1000.0 / frequency_hz)));` |
| `CommandExecutor::try_flush_msgs` | 336 | `void CommandExecutor::try_flush_msgs()` |
| `std::move` | 350 | `msg = std::move(pending_msgs_.front());` |
| `std::move` | 359 | `pending_msgs_.push_front(std::move(msg));` |
| `CommandExecutor::launch_background_task` | 365 | `void CommandExecutor::launch_background_task(std::thread&& worker) {` |
| `std::move` | 367 | `background_workers_.emplace_back(std::move(worker));` |
| `CommandExecutor::log_runtime_flags` | 370 | `void CommandExecutor::log_runtime_flags(const char* stage) const {` |
| `CommandExecutor::is_pid_still_running` | 395 | `bool CommandExecutor::is_pid_still_running(pid_t pid) const {` |
| `std::strerror` | 408 | `pid, errno, std::strerror(errno));` |
| `CommandExecutor::is_navigation_environment_running` | 414 | `bool CommandExecutor::is_navigation_environment_running(std::string& reason) {` |
| `std::to_string` | 422 | `reason = "navigation process is still running, pid=" + std::to_string(nav_pid);` |
| `CommandExecutor::log_wait_status` | 456 | `void CommandExecutor::log_wait_status(const std::string& name, int status) const {` |
| `std::strerror` | 459 | `name, errno, std::strerror(errno));` |
| `CommandExecutor::log_file_access` | 479 | `bool CommandExecutor::log_file_access(const std::string& name, const std::string& path, int mode) const {` |
| `std::strerror` | 487 | `name, path, mode, errno, std::strerror(errno));` |
| `CommandExecutor::shell_quote` | 491 | `std::string CommandExecutor::shell_quote(const std::string& value) const {` |
| `CommandExecutor::script_log_command` | 504 | `std::string CommandExecutor::script_log_command(const std::string& script_path, const std::string& tag) const {` |
| `LogManager::instance` | 505 | `const std::string log_path = LogManager::instance().get_log_file_path();` |
| `CommandExecutor::ros_prepare` | 515 | `void CommandExecutor::ros_prepare(const json& payload_json) {` |
| `CommandExecutor::loc_callback` | 519 | `void CommandExecutor::loc_callback(const std_msgs::Int8::ConstPtr& msg) {` |
| `CommandExecutor::is_managed_command` | 533 | `bool CommandExecutor::is_managed_command(const std::string& cmd_type) const {` |
| `CommandExecutor::try_begin_command` | 550 | `bool CommandExecutor::try_begin_command(const std::string& cmd_type) {` |
| `CommandExecutor::finish_command` | 574 | `void CommandExecutor::finish_command(const std::string& cmd_type) {` |
| `CommandExecutor::execute` | 594 | `void CommandExecutor::execute(const json& payload_json) {` |
| `json::parse` | 598 | `? json::parse(payload_json.get<std::string>())` |
| `CommandExecutor::robot_bt` | 653 | `void CommandExecutor::robot_bt(std::string cmd_type, json data) {` |
| `std::thread` | 808 | `launch_background_task(std::thread([this, clear_attempt_id]() {` |
| `this_thread::sleep_for` | 809 | `std::this_thread::sleep_for(std::chrono::seconds(kClearWaypointsAckTimeoutSec_));` |
| `CommandExecutor::publish_fsm_state` | 887 | `void CommandExecutor::publish_fsm_state(const std::string& command, const std::string& status,` |
| `RobotFsm::name` | 891 | `{"device_id", device_id_}, {"motion_mode", RobotFsm::name(state.motion_mode)},` |
| `RobotFsm::task_name` | 892 | `{"task_status", RobotFsm::task_name(state.task_status)},` |
| `RobotFsm::name` | 899 | `command, status, RobotFsm::name(state.motion_mode), state.generation, message);` |
| `CommandExecutor::admit_behavior_command` | 902 | `bool CommandExecutor::admit_behavior_command(const std::string& type) {` |
| `RobotFsm::is_navigation` | 910 | `allowed = RobotFsm::is_navigation(state.motion_mode);` |
| `CommandExecutor::invalidate_navigation_requests` | 922 | `void CommandExecutor::invalidate_navigation_requests() {` |
| `CommandExecutor::handle_follow_command` | 960 | `void CommandExecutor::handle_follow_command(const std::string& type, const json& data) {` |
| `CommandExecutor::get_robot_position` | 993 | `bool CommandExecutor::get_robot_position(double& x, double& y, double& z)` |
| `ros::Time` | 1018 | `if (!tf_listener_.canTransform(target_frame, source_frame, ros::Time(0)))` |
| `ros::Time` | 1023 | `tf_listener_.canTransform(target_frame, source_frame, ros::Time(0), &tf_error);` |
| `ros::Time` | 1034 | `tf_listener_.lookupTransform(target_frame, source_frame, ros::Time(0), transform);` |
| `CommandExecutor::get_robot_orientation` | 1077 | `bool CommandExecutor::get_robot_orientation(double& roll, double& pitch, double& yaw) {` |
| `ros::Time` | 1103 | `if (!tf_listener_.canTransform(target_frame, source_frame, ros::Time(0), &tf_error)) {` |
| `ros::Time` | 1115 | `tf_listener_.lookupTransform(target_frame, source_frame, ros::Time(0), transform);` |
| `CommandExecutor::global_path_callback` | 1176 | `void CommandExecutor::global_path_callback(const nav_msgs::Path::ConstPtr& msg) {` |
| `system_clock::now` | 1195 | `const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();` |
| `std::to_string` | 1201 | `{"path_id", "path_" + std::to_string(timestamp_ms)},` |
| `CommandExecutor::localization_callback` | 1231 | `void CommandExecutor::localization_callback(const nav_msgs::Odometry::ConstPtr& msg) {` |
| `system_clock::now` | 1246 | `const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();` |
| `CommandExecutor::initial_pose_rosout_callback` | 1260 | `void CommandExecutor::initial_pose_rosout_callback(const rosgraph_msgs::Log::ConstPtr& msg) {` |
| `RobotFsm::is_navigation` | 1262 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `std::strtoll` | 1376 | `const long long waypoint_index = std::strtoll(` |
| `std::min` | 1401 | `reached_points = std::min(reached_points, total_points);` |
| `std::max` | 1402 | `task_execution_reached_points_ = std::max(` |
| `std::max` | 1407 | `task_execution_reached_points_ = std::max(` |
| `fmt::format` | 1427 | `const std::string progress = fmt::format("{}/{}", reached_points, total_points);` |
| `fmt::format` | 1431 | `waypoint_msg["progress_message"] = fmt::format("导航任务点已经完成了 {}", progress);` |
| `CommandExecutor::handle_navigation_planning_rosout` | 1557 | `void CommandExecutor::handle_navigation_planning_rosout(const std::string& message) {` |
| `std::sscanf` | 1582 | `const int matched = std::sscanf(coords.c_str(), "x=%lf, y=%lf, z=%lf", &x, &y, &z);` |
| `CommandExecutor::send_navigation_planning_failure` | 1608 | `void CommandExecutor::send_navigation_planning_failure() {` |
| `CommandExecutor::complete_initial_pose_result` | 1663 | `void CommandExecutor::complete_initial_pose_result(unsigned long long attempt_id, bool success, const std::string& message) {` |
| `RobotFsm::is_navigation` | 1665 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `CommandExecutor::complete_navigation_env_ready` | 1700 | `void CommandExecutor::complete_navigation_env_ready(unsigned long long attempt_id, bool success, const std::string& message) {` |
| `RobotFsm::is_navigation` | 1710 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `CommandExecutor::complete_navigation_task_result` | 1746 | `void CommandExecutor::complete_navigation_task_result(unsigned long long attempt_id, bool success, const std::string& message) {` |
| `RobotFsm::is_navigation` | 1748 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `CommandExecutor::complete_clear_waypoints_result` | 1786 | `void CommandExecutor::complete_clear_waypoints_result(unsigned long long attempt_id, bool success, const std::string& message) {` |
| `RobotFsm::is_navigation` | 1788 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `CommandExecutor::telemetry_loop` | 1819 | `void CommandExecutor::telemetry_loop() {` |
| `ros::ok` | 1821 | `while (ros::ok()) {` |
| `system_clock::now` | 1828 | `const auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();` |
| `CommandExecutor::handle_initial_pose` | 1845 | `void CommandExecutor::handle_initial_pose(const json& data)` |
| `tf::Matrix3x3` | 1873 | `tf::Matrix3x3(q).getRPY(roll, pitch, yaw);` |
| `fmt::format` | 1880 | `std::string final_cmd = fmt::format(` |
| `std::thread` | 1893 | `std::thread([final_cmd]() {` |
| `std::system` | 1895 | `int ret = std::system(final_cmd.c_str());` |
| `std::thread` | 1904 | `launch_background_task(std::thread([this, attempt_id]() {` |
| `this_thread::sleep_for` | 1905 | `std::this_thread::sleep_for(std::chrono::seconds(kInitialPoseTimeoutSec_));` |
| `CommandExecutor::handle_start_navigation` | 1914 | `void CommandExecutor::handle_start_navigation(const json& data) {` |
| `fs::remove` | 1965 | `fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/navmap.pcd");` |
| `fs::remove` | 1966 | `fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomogram/navmap.pickle");` |
| `fs::path` | 1969 | `fs::path file_path = fs::path("/home/jianmi/map_manager_local") / uuid;` |
| `fs::copy` | 1970 | `fs::copy(` |
| `fs::copy` | 1976 | `fs::copy(` |
| `fs::copy` | 1983 | `fs::copy(` |
| `fs::copy` | 1989 | `fs::copy(` |
| `std::thread` | 2028 | `std::thread(` |
| `CommandExecutor::start_navigation_async` | 2039 | `void CommandExecutor::start_navigation_async(json data, std::string uuid_old, std::string uuid,` |
| `fs::path` | 2051 | `fs::path file_path = fs::path(MAP_MANAGER_LOCAL) / uuid;` |
| `fmt::format` | 2105 | `std::string nav_command = fmt::format(` |
| `std::strerror` | 2125 | `errno, std::strerror(errno));` |
| `std::fprintf` | 2151 | `std::fprintf(stderr, "[NavigationEnv] execl failed for navigation.sh, path=%s, errno=%d, error=%s\n",` |
| `std::strerror` | 2152 | `nav_script.c_str(), errno, std::strerror(errno));` |
| `std::thread` | 2173 | `launch_background_task(std::thread([this, nav_env_attempt_id]() {` |
| `this_thread::sleep_for` | 2174 | `std::this_thread::sleep_for(std::chrono::seconds(kNavigationEnvReadyTimeoutSec_));` |
| `CommandExecutor::handle_navigation_task` | 2211 | `void CommandExecutor::handle_navigation_task(const json& data) {` |
| `json::array` | 2215 | `const json &points = data.value("target_points", json::array());` |
| `std::thread` | 2245 | `launch_background_task(std::thread([this, nav_task_attempt_id]() {` |
| `this_thread::sleep_for` | 2246 | `std::this_thread::sleep_for(std::chrono::seconds(kNavigationTaskAckTimeoutSec_));` |
| `ros::Duration` | 2259 | `ros::Duration(0.05).sleep();` |
| `Time::now` | 2265 | `// target.header.stamp = ros::Time::now();` |
| `ros::Time` | 2266 | `target.header.stamp = ros::Time(pt.value("wait_time", 0.0));` |
| `CommandExecutor::handle_set_dock_pose` | 2291 | `void CommandExecutor::handle_set_dock_pose(const json& data) {` |
| `json::object` | 2330 | `json orientation_json = json::object();` |
| `Time::now` | 2347 | `dock_pose.header.stamp = ros::Time::now();` |
| `std::string` | 2352 | `: std::string("map"));` |
| `CommandExecutor::handle_stop_navigation` | 2381 | `bool CommandExecutor::handle_stop_navigation(const json& data) {` |
| `this_thread::sleep_for` | 2401 | `std::this_thread::sleep_for(std::chrono::milliseconds(100));` |
| `this_thread::sleep_for` | 2407 | `std::this_thread::sleep_for(std::chrono::milliseconds(100));` |
| `this_thread::sleep_for` | 2423 | `std::this_thread::sleep_for(std::chrono::milliseconds(100));` |
| `this_thread::sleep_for` | 2429 | `std::this_thread::sleep_for(std::chrono::milliseconds(100));` |
| `std::fprintf` | 2449 | `std::fprintf(stderr, "[StopNavigation] execl failed for kill_all.sh, path=%s, errno=%d, error=%s\n",` |
| `std::strerror` | 2450 | `kill_scripts.c_str(), errno, std::strerror(errno));` |
| `std::strerror` | 2463 | `pid, errno, std::strerror(errno));` |
| `this_thread::sleep_for` | 2466 | `std::this_thread::sleep_for(std::chrono::seconds(1));` |
| `std::strerror` | 2480 | `errno, std::strerror(errno));` |
| `CommandExecutor::handle_start_mapping` | 2537 | `void CommandExecutor::handle_start_mapping(const json& data) {` |
| `CommandExecutor::handle_start_sh` | 2550 | `void CommandExecutor::handle_start_sh(const json& data)` |
| `std::getline` | 2567 | `while (std::getline(ss, item, '/')) {` |
| `fs::copy` | 2584 | `fs::copy(` |
| `fs::path` | 2586 | `fs::path("/home/jianmi/g_nav_map") / "gloabl_map.pcd",` |
| `fs::copy` | 2590 | `fs::copy(` |
| `fs::path` | 2592 | `fs::path("/home/jianmi/g_nav_map") / "gloabl_map.pickle",` |
| `this_thread::sleep_for` | 2609 | `std::this_thread::sleep_for(std::chrono::seconds(2));` |
| `fs::path` | 2615 | `fs::path("/home/jianmi/map_manager_local")` |
| `fs::create_directories` | 2618 | `fs::create_directories(uuid_dir);` |
| `fs::copy` | 2622 | `fs::copy(` |
| `fs::copy` | 2628 | `fs::copy(` |
| `fs::copy` | 2634 | `fs::copy(` |
| `fs::copy` | 2640 | `fs::copy(` |
| `CommandExecutor::exec_upload_cmd` | 2657 | `void CommandExecutor::exec_upload_cmd(const json& data) {` |
| `CommandExecutor::handle_stop_and_uploadmap` | 2664 | `void CommandExecutor::handle_stop_and_uploadmap(const json& data) {` |
| `std::thread` | 2677 | `launch_background_task(std::thread(&CommandExecutor::stop_and_uploadmap_async, this, data));` |
| `CommandExecutor::stop_and_uploadmap_async` | 2680 | `void CommandExecutor::stop_and_uploadmap_async(json data) {` |
| `CommandExecutor::get_last_storage_path` | 2689 | `std::string CommandExecutor::get_last_storage_path() const {` |
| `CommandExecutor::set_upload_enable` | 2693 | `void CommandExecutor::set_upload_enable(bool enable) {` |
| `CommandExecutor::get_localization_ready` | 2697 | `bool CommandExecutor::get_localization_ready() const {` |
| `CommandExecutor::get_current_map_id` | 2701 | `std::string CommandExecutor::get_current_map_id() const {` |
| `CommandExecutor::set_current_map_id` | 2706 | `void CommandExecutor::set_current_map_id(const std::string& id) {` |
| `CommandExecutor::get_state_name` | 2711 | `std::string CommandExecutor::get_state_name() const {` |
| `RobotFsm::name` | 2712 | `return RobotFsm::name(behavior_fsm_.snapshot().motion_mode);` |
| `CommandExecutor::enqueue_task_state_request` | 2715 | `void CommandExecutor::enqueue_task_state_request(` |
| `CommandExecutor::process_next_task_state_request` | 2734 | `void CommandExecutor::process_next_task_state_request() {` |
| `CommandExecutor::complete_task_state_transition` | 2780 | `void CommandExecutor::complete_task_state_transition(bool success, int ret) {` |
| `RobotFsm::is_navigation` | 2782 | `if (!RobotFsm::is_navigation(behavior_fsm_.snapshot().motion_mode)) return;` |
| `CommandExecutor::exec_rosservice_cmd` | 2821 | `void CommandExecutor::exec_rosservice_cmd(const std::string& type, const std::string& service_name) {` |
| `std::thread` | 2829 | `launch_background_task(std::thread([this, final_cmd, type, service_name, generation]() {` |
| `std::system` | 2834 | `int ret = std::system(final_cmd.c_str());` |

### src/aiRecognition/AiRecognitionAckHandler.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `std::isalnum` | 22 | `result += (std::isalnum(c) \|\| c == '-' \|\| c == '_') ? static_cast<char>(c) : '_';` |
| `std::remove_if` | 31 | `input.erase(std::remove_if(input.begin(), input.end(), [](unsigned char c) {` |
| `std::isspace` | 32 | `return std::isspace(c);` |
| `AiRecognitionAckHandler::AiRecognitionAckHandler` | 59 | `AiRecognitionAckHandler::AiRecognitionAckHandler(CommandExecutor& executor, std::string device_id, std::string image_directory)` |
| `std::move` | 60 | `: executor_(executor) , device_id_(std::move(device_id)), image_directory_(std::move(image_directory)) {}` |
| `AiRecognitionAckHandler::handle` | 62 | `void AiRecognitionAckHandler::handle(const std::string& payload, bool binary) const {` |
| `json::parse` | 67 | `ack = json::parse(payload);` |
| `filesystem::create_directories` | 83 | `std::filesystem::create_directories(image_directory_);` |
| `std::to_string` | 107 | `// const auto filename = ack_id + "_" + std::to_string(seq) + "_" + algorithm_type + "_" + std::to_string(timestamp) + "_" + std::to_string(index) + image_extension(image);` |
| `std::to_string` | 108 | `const auto filename = algorithm_type + "_" + std::to_string(timestamp) + "_" + std::to_string(index) + image_extension(image); // visual_inspection_1791884368_0.jpg` |
| `filesystem::path` | 109 | `const auto path = std::filesystem::path(image_directory_) / filename;` |

### src/aiRecognition/AiRecognitionAckHandler.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `AiRecognitionAckHandler` | 9 | `AiRecognitionAckHandler(CommandExecutor& executor, std::string device_id, std::string image_directory);` |
| `handle` | 11 | `void handle(const std::string& payload, bool binary) const;` |

### src/bms/BmsForwarder.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `system_clock::now` | 39 | `std::chrono::system_clock::now().time_since_epoch()).count();` |
| `system_clock::now` | 44 | `std::chrono::system_clock::now().time_since_epoch()).count();` |
| `json::array` | 93 | `json active_alarm_levels = json::array();` |
| `json::array` | 111 | `json active_flags = json::array();` |
| `json::array` | 139 | `const json alarm_flags = json::array({` |
| `std::move` | 283 | `websocket_url_(std::move(websocket_url)),` |
| `std::move` | 284 | `robot_id_(std::move(robot_id)),` |
| `std::move` | 285 | `bms_topic_(bms_topic.empty() ? "/m20/bms_state" : std::move(bms_topic)),` |
| `std::move` | 286 | `robot_model_(std::move(robot_model)) {` |
| `std::transform` | 287 | `std::transform(robot_model_.begin(), robot_model_.end(), robot_model_.begin(),` |
| `std::toupper` | 288 | `[](unsigned char c) { return std::toupper(c); });` |
| `std::thread` | 335 | `ws_thread_ = std::thread([this]() {` |
| `asio::post` | 339 | `boost::asio::post(ws_client_.get_io_service(), [this]() {` |
| `json::array` | 368 | `json flags = json::array();` |
| `std::min` | 383 | `if (present1 && present2) soc = std::min(double(msg->bat01.soc_percent), double(msg->bat02.soc_percent));` |
| `asio::post` | 441 | `boost::asio::post(ws_client_.get_io_service(), [this]() {` |
| `BmsForwarder::BmsForwarder` | 648 | `BmsForwarder::BmsForwarder(ros::NodeHandle& nh, std::string websocket_url,` |
| `std::move` | 651 | `nh, std::move(websocket_url), std::move(robot_id),` |
| `std::move` | 652 | `std::move(bms_topic), std::move(robot_model))) {}` |
| `BmsForwarder::~BmsForwarder` | 654 | `BmsForwarder::~BmsForwarder() = default;` |
| `BmsForwarder::start` | 656 | `bool BmsForwarder::start() {` |
| `BmsForwarder::stop` | 660 | `void BmsForwarder::stop() {` |
| `BmsForwarder::setBatteryCallback` | 664 | `void BmsForwarder::setBatteryCallback(std::function<void(bool, double)> callback) {` |
| `std::move` | 665 | `impl_->battery_callback_ = std::move(callback);` |

### src/bms/BmsForwarder.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `BmsForwarder` | 11 | `BmsForwarder(ros::NodeHandle& nh, std::string websocket_url,` |
| `~BmsForwarder` | 14 | `~BmsForwarder();` |
| `BmsForwarder` | 16 | `BmsForwarder(const BmsForwarder&) = delete;` |
| `start` | 19 | `bool start();` |
| `stop` | 20 | `void stop();` |
| `setBatteryCallback` | 22 | `void setBatteryCallback(std::function<void(bool, double)> callback);` |

### src/broadcast/broadcastHandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `Broadcast::Broadcast` | 5 | `Broadcast::Broadcast(std::string device_id, std::string http_ip, int http_port,` |
| `std::move` | 8 | `: device_id_(std::move(device_id)),` |
| `std::move` | 9 | `http_ip_(std::move(http_ip)),` |
| `std::move` | 10 | `http_port_(std::move(http_port)),` |
| `std::move` | 11 | `send_to_cloud_(std::move(send_to_cloud)) {}` |
| `Broadcast::download_broadcast` | 14 | `bool Broadcast::download_broadcast(const std::string& filename, const std::string& http_ip, const int& http_port) {` |
| `std::to_string` | 15 | `std::string cmd = "curl -f -L http://" + http_ip + ":" + std::to_string(http_port) +` |

### src/broadcast/broadcastHandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `Broadcast` | 14 | `Broadcast(std::string device_id, std::string http_ip, int http_port,` |
| `download_broadcast` | 17 | `bool download_broadcast(const std::string& filename, const std::string& http_ip, const int& http_port);` |

### src/follow/followHandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `Follow::Follow` | 24 | `Follow::Follow(std::string device_id, SendToCloud send_to_cloud, std::string base_url)` |
| `std::move` | 25 | `: device_id_(std::move(device_id)),` |
| `std::move` | 26 | `send_to_cloud_(std::move(send_to_cloud)),` |
| `std::move` | 27 | `base_url_(std::move(base_url))` |
| `Follow::handle_follow_function` | 34 | `void Follow::handle_follow_function(const json& request)` |
| `json::object` | 36 | `json response = json::object();` |
| `std::runtime_error` | 39 | `throw std::runtime_error("follow request must be a JSON object");` |
| `std::runtime_error` | 54 | `throw std::runtime_error("unknown follow cmd: " + command);` |
| `std::runtime_error` | 58 | `throw std::runtime_error("HTTP initialization failed");` |
| `std::runtime_error` | 63 | `throw std::runtime_error("HTTP client initialization failed");` |
| `std::runtime_error` | 69 | `throw std::runtime_error("HTTP header allocation failed");` |
| `std::runtime_error` | 95 | `throw std::runtime_error(std::string("follow HTTP failed: ") +` |
| `json::parse` | 100 | `response = json::parse(reply, nullptr, false);` |
| `json::object` | 102 | `response = json::object();` |
| `std::runtime_error` | 103 | `throw std::runtime_error("follow backend returned invalid JSON; HTTP " +` |
| `std::to_string` | 104 | `std::to_string(status));` |
| `std::to_string` | 109 | `response["msg"] = "follow HTTP status " + std::to_string(status);` |
| `std::runtime_error` | 113 | `throw std::runtime_error("follow backend response requires boolean ok");` |

### src/follow/followHandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `Follow` | 15 | `Follow(std::string device_id,` |
| `handle_follow_function` | 19 | `void handle_follow_function(const json& data);` |

### src/gateway/gateway.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `gateway::gateway` | 5 | `gateway::gateway()` |
| `asio::bind_executor` | 31 | `ws_server.set_open_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_server_open, this, std::placeholders::_1)));` |
| `asio::bind_executor` | 32 | `ws_server.set_close_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_server_close, this, std::placeholders::_1)));` |
| `asio::bind_executor` | 33 | `ws_server.set_message_handler(boost::asio::bind_executor(*server_strand, std::bind(&gateway::on_message, this, std::placeholders::_1, std::placeholders::_2)));` |
| `asio::bind_executor` | 35 | `ws_client.set_open_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_open, this, std::placeholders::_1)));` |
| `asio::bind_executor` | 36 | `ws_client.set_close_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_close, this, std::placeholders::_1)));` |
| `asio::bind_executor` | 37 | `ws_client.set_message_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_message, this, std::placeholders::_1, std::placeholders::_2)));` |
| `asio::bind_executor` | 38 | `ws_client.set_fail_handler(boost::asio::bind_executor(*client_strand, std::bind(&gateway::on_client_fail, this, std::placeholders::_1)));` |
| `std::bind` | 41 | `ws_client.set_pong_handler(std::bind(&gateway::on_pong, this, std::placeholders::_1, std::placeholders::_2));` |
| `std::bind` | 42 | `ws_client.set_pong_timeout_handler(std::bind(` |
| `gateway::~gateway` | 49 | `gateway::~gateway()` |
| `gateway::start_server` | 72 | `bool gateway::start_server(uint16_t port)` |
| `tcp::v4` | 77 | `ws_server.listen(boost::asio::ip::tcp::v4(), port);` |
| `std::thread` | 81 | `server_thread = std::thread([this, port]()` |
| `gateway::start_client` | 95 | `bool gateway::start_client(const std::string &URL)` |
| `std::thread` | 102 | `client_thread = std::thread([this]()` |
| `this_thread::get_id` | 104 | `client_thread_id = std::this_thread::get_id();` |
| `asio::post` | 109 | `boost::asio::post(*client_strand, [this]()` |
| `gateway::connect_heartbeat` | 114 | `void gateway::connect_heartbeat()` |
| `gateway::setup` | 124 | `bool gateway::setup(const std::string &web_url)` |
| `gateway::schedule_reconnect` | 157 | `void gateway::schedule_reconnect()` |
| `std::min` | 184 | `delay_ms = std::min(delay_ms, reconnect_max_delay_ms_);` |
| `asio::bind_executor` | 191 | `ws_client.set_timer(delay_ms, boost::asio::bind_executor(*client_strand,` |
| `gateway::connect_to_web` | 206 | `void gateway::connect_to_web()` |
| `gateway::is_same_hdl` | 244 | `bool gateway::is_same_hdl(connection_hdl lhs, connection_hdl rhs) const` |
| `gateway::update_web_endpoint` | 249 | `void gateway::update_web_endpoint(const std::string& url)` |
| `gateway::tcp_probe` | 289 | `bool gateway::tcp_probe(const std::string& host, const std::string& service,` |
| `steady_clock::now` | 293 | `const auto started = std::chrono::steady_clock::now();` |
| `chrono::milliseconds` | 302 | `timer.expires_after(std::chrono::milliseconds(timeout_ms));` |
| `asio::async_connect` | 325 | `boost::asio::async_connect(socket, endpoints,` |
| `steady_clock::now` | 338 | `std::chrono::steady_clock::now() - started).count();` |
| `gateway::run_network_diagnostics` | 350 | `void gateway::run_network_diagnostics(const char* reason)` |
| `std::thread` | 363 | `std::thread([this, reason_copy, target_host, target_port, url]() {` |
| `gateway::close_web_connection` | 400 | `bool gateway::close_web_connection(const char* reason)` |
| `lib::error_code` | 422 | `con->terminate(websocketpp::lib::error_code());` |
| `gateway::mark_web_disconnected` | 426 | `void gateway::mark_web_disconnected(const char* reason, bool reconnect)` |
| `this_thread::get_id` | 441 | `if (std::this_thread::get_id() == client_thread_id) {` |
| `asio::post` | 444 | `boost::asio::post(*client_strand, cleanup_and_reconnect);` |
| `gateway::send_to_web_now` | 448 | `bool gateway::send_to_web_now(const std::string& payload)` |
| `gateway::schedule_heartbeat` | 526 | `void gateway::schedule_heartbeat(int generation)` |
| `asio::bind_executor` | 532 | `ws_client.set_timer(pingSecond * 1000, boost::asio::bind_executor(*client_strand,` |
| `system_clock::now` | 552 | `std::chrono::system_clock::now().time_since_epoch()).count();` |
| `steady_clock::now` | 565 | `std::chrono::steady_clock::now().time_since_epoch()).count();` |
| `std::to_string` | 567 | `ws_client.ping(hdl_heartbeat, std::to_string(ping_timestamp_ms), ping_ec);` |
| `gateway::set_device_id` | 578 | `void gateway::set_device_id(const std::string& device_id)` |
| `gateway::send_heartbeat_now` | 583 | `bool gateway::send_heartbeat_now(const std::string& payload)` |
| `gateway::on_server_open` | 599 | `void gateway::on_server_open(connection_hdl hdl)` |
| `gateway::on_server_close` | 606 | `void gateway::on_server_close(connection_hdl hdl)` |
| `gateway::on_client_open` | 612 | `void gateway::on_client_open(connection_hdl hdl)` |
| `steady_clock::now` | 633 | `last_web_packet_time = std::chrono::steady_clock::now();` |
| `asio::bind_executor` | 635 | `ws_client.set_timer(30000, boost::asio::bind_executor(*client_strand,` |
| `tcp::no_delay` | 656 | `con->get_socket().set_option(boost::asio::ip::tcp::no_delay(true));` |
| `socket::keep_alive` | 657 | `con->get_socket().set_option(boost::asio::ip::tcp::socket::keep_alive(true));` |
| `gateway::on_client_close` | 677 | `void gateway::on_client_close(connection_hdl hdl)` |
| `asio::bind_executor` | 683 | `ws_client.set_timer(2000, boost::asio::bind_executor(*client_strand,` |
| `std::min` | 705 | `kicked_reconnect_attempts_ = std::min(kicked_reconnect_attempts_ + 1, 6);` |
| `std::max` | 706 | `reconnect_attempts_ = std::max(reconnect_attempts_, kicked_reconnect_attempts_);` |
| `gateway::on_client_fail` | 714 | `void gateway::on_client_fail(connection_hdl hdl)` |
| `gateway::on_pong` | 733 | `void gateway::on_pong(connection_hdl hdl, std::string payload)` |
| `steady_clock::now` | 735 | `last_web_pong_time = std::chrono::steady_clock::now();` |
| `steady_clock::now` | 738 | `auto now = std::chrono::steady_clock::now();` |
| `std::stoll` | 741 | `long long sent_ms = std::stoll(payload);` |
| `gateway::on_pong_timeout` | 755 | `void gateway::on_pong_timeout(connection_hdl hdl, std::string payload)` |
| `gateway::is_duplicate_command` | 768 | `bool gateway::is_duplicate_command(const json& payload_json)` |
| `json::array` | 795 | `dedup_key = json::array({type, device_id, seq, id}).dump();` |
| `gateway::is_command_expired` | 850 | `bool gateway::is_command_expired(const json& payload_json, long long local_now_ms, long long* age_ms) const` |
| `gateway::broadcast_data` | 869 | `void gateway::broadcast_data(const std::string &payload)` |
| `asio::post` | 871 | `boost::asio::post(*server_strand, [this, payload]()` |
| `asio::post` | 885 | `boost::asio::post(*client_strand, [this, payload]()` |
| `gateway::on_message` | 891 | `void gateway::on_message(connection_hdl hdl, server::message_ptr msg)` |
| `steady_clock::now` | 893 | `auto t1 = std::chrono::steady_clock::now();` |
| `system_clock::now` | 920 | `auto now = std::chrono::system_clock::now();` |
| `steady_clock::now` | 921 | `this->last_web_packet_time = std::chrono::steady_clock::now();` |
| `json::parse` | 927 | `payload_json = json::parse(payload);` |
| `std::thread` | 1034 | `std::thread(invoke_callback).detach();` |
| `asio::post` | 1043 | `boost::asio::post(` |
| `steady_clock::now` | 1057 | `auto t2 = std::chrono::steady_clock::now();` |
| `gateway::setMessageCallback` | 1073 | `void gateway::setMessageCallback(MessageCallback cb)` |
| `std::move` | 1076 | `message_callback_ = std::move(cb);` |
| `gateway::setBroadcastCallback` | 1079 | `void gateway::setBroadcastCallback(std::function<void(const std::string&)> cb)` |
| `std::move` | 1082 | `broadcast_callback_ = std::move(cb);` |
| `gateway::setConnectedCallback` | 1085 | `void gateway::setConnectedCallback(std::function<void()> cb)` |
| `std::move` | 1088 | `connected_callback_ = std::move(cb);` |
| `gateway::isWebConnected` | 1091 | `bool gateway::isWebConnected() const` |
| `gateway::sendToWeb` | 1096 | `bool gateway::sendToWeb(const std::string& payload)` |
| `this_thread::get_id` | 1105 | `if (std::this_thread::get_id() == client_thread_id) {` |
| `asio::post` | 1111 | `boost::asio::post(*client_strand, [this, payload, promise]()` |
| `chrono::milliseconds` | 1122 | `if (future.wait_for(std::chrono::milliseconds(2000)) != std::future_status::ready) {` |
| `gateway::state_name` | 1136 | `std::string gateway::state_name(GatewayState state) const` |
| `gateway::update_state` | 1149 | `void gateway::update_state(GatewayState new_state, const char* reason)` |
| `gateway::get_state_name` | 1167 | `std::string gateway::get_state_name() const` |

### src/gateway/gateway.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `gateway` | 44 | `gateway();` |
| `~gateway` | 45 | `~gateway();` |
| `setup` | 47 | `bool setup(const std::string &web_url);` |
| `set_device_id` | 49 | `void set_device_id(const std::string& device_id);` |
| `setMessageCallback` | 51 | `void setMessageCallback(MessageCallback cb);` |
| `setBroadcastCallback` | 52 | `void setBroadcastCallback(std::function<void(const std::string&)> cb);` |
| `setConnectedCallback` | 53 | `void setConnectedCallback(std::function<void()> cb);` |
| `broadcast_data` | 54 | `void broadcast_data(const std::string &payload);` |
| `isWebConnected` | 56 | `bool isWebConnected() const;` |
| `sendToWeb` | 57 | `bool sendToWeb(const std::string& payload);` |
| `get_state_name` | 58 | `std::string get_state_name() const;` |
| `on_server_open` | 118 | `void on_server_open(connection_hdl hdl);` |
| `on_server_close` | 119 | `void on_server_close(connection_hdl hdl);` |
| `on_client_open` | 120 | `void on_client_open(connection_hdl hdl);` |
| `on_client_close` | 121 | `void on_client_close(connection_hdl hdl);` |
| `on_client_fail` | 122 | `void on_client_fail(connection_hdl hdl);` |
| `on_pong` | 123 | `void on_pong(connection_hdl hdl, std::string payload);` |
| `on_pong_timeout` | 124 | `void on_pong_timeout(connection_hdl hdl, std::string payload);` |
| `on_message` | 125 | `void on_message(connection_hdl hdl, server::message_ptr msg);` |
| `connect_to_web` | 127 | `void connect_to_web();` |
| `connect_heartbeat` | 128 | `void connect_heartbeat();` |
| `schedule_reconnect` | 129 | `void schedule_reconnect();` |
| `schedule_heartbeat` | 130 | `void schedule_heartbeat(int generation);` |
| `close_web_connection` | 131 | `bool close_web_connection(const char* reason);` |
| `is_same_hdl` | 132 | `bool is_same_hdl(connection_hdl lhs, connection_hdl rhs) const;` |
| `send_to_web_now` | 133 | `bool send_to_web_now(const std::string& payload);` |
| `send_heartbeat_now` | 134 | `bool send_heartbeat_now(const std::string& payload);` |
| `mark_web_disconnected` | 135 | `void mark_web_disconnected(const char* reason, bool reconnect);` |
| `run_network_diagnostics` | 136 | `void run_network_diagnostics(const char* reason);` |
| `update_web_endpoint` | 137 | `void update_web_endpoint(const std::string& url);` |
| `tcp_probe` | 138 | `static bool tcp_probe(const std::string& host, const std::string& service,` |
| `is_duplicate_command` | 141 | `bool is_duplicate_command(const json& payload_json);` |
| `is_command_expired` | 142 | `bool is_command_expired(const json& payload_json, long long local_now_ms, long long* age_ms) const;` |
| `update_state` | 143 | `void update_state(GatewayState new_state, const char* reason);` |
| `state_name` | 144 | `std::string state_name(GatewayState state) const;` |
| `start_server` | 146 | `bool start_server(uint16_t port);` |
| `start_client` | 147 | `bool start_client(const std::string &URL);` |

### src/hikvision/hikvihandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `std::call_once` | 30 | `std::call_once(flag, []() {` |
| `std::isspace` | 64 | `while (std::isspace(static_cast<unsigned char>(*text))) {` |
| `std::strtod` | 69 | `const double parsed = std::strtod(text, &end);` |
| `std::isfinite` | 70 | `if (end == text \|\| !std::isfinite(parsed)) {` |
| `std::isspace` | 73 | `while (std::isspace(static_cast<unsigned char>(*end))) {` |
| `std::setprecision` | 180 | `output << std::fixed << std::setprecision(2) << value;` |
| `std::fmod` | 185 | `value = std::fmod(value, 360.0);` |
| `std::stod` | 279 | `angle = std::stod(text, &parsed_chars);` |
| `HikvisionHandle::HikvisionHandle` | 290 | `HikvisionHandle::HikvisionHandle(std::string ip,` |
| `std::move` | 294 | `: ip_(std::move(ip)),` |
| `std::move` | 295 | `username_(std::move(username)),` |
| `std::move` | 296 | `password_(std::move(password)),` |
| `std::to_string` | 300 | `std::to_string(channel_) + "/absoluteEx";` |
| `std::to_string` | 303 | `std::to_string(channel_) + "/continuous";` |
| `std::thread` | 305 | `std::thread(&HikvisionHandle::continuous_watchdog_loop, this);` |
| `HikvisionHandle::~HikvisionHandle` | 308 | `HikvisionHandle::~HikvisionHandle() {` |
| `HikvisionHandle::request` | 325 | `HikvisionHandle::HttpResponse HikvisionHandle::request(` |
| `HikvisionHandle::get_position` | 374 | `bool HikvisionHandle::get_position(Position& position,` |
| `std::to_string` | 383 | `std::to_string(response.status_code);` |
| `HikvisionHandle::get_capabilities` | 399 | `bool HikvisionHandle::get_capabilities(Capabilities& capabilities,` |
| `std::to_string` | 408 | `std::to_string(response.status_code);` |
| `HikvisionHandle::set_position` | 424 | `bool HikvisionHandle::set_position(const Position& position,` |
| `std::to_string` | 447 | `std::to_string(response.status_code);` |
| `HikvisionHandle::set_continuous_velocity` | 453 | `bool HikvisionHandle::set_continuous_velocity(int pan_speed,` |
| `std::to_string` | 473 | `std::to_string(response.status_code);` |
| `HikvisionHandle::continuous_controller` | 479 | `HikvisionHandle::Result HikvisionHandle::continuous_controller(` |
| `steady_clock::now` | 512 | `last_continuous_command_time_ = std::chrono::steady_clock::now();` |
| `HikvisionHandle::execute_continuous_command` | 522 | `json HikvisionHandle::execute_continuous_command(` |
| `HikvisionHandle::handle_continuous_command` | 560 | `json HikvisionHandle::handle_continuous_command(const json& data) {` |
| `HikvisionHandle::handle_command` | 570 | `json HikvisionHandle::handle_command(const json& data) {` |
| `std::isfinite` | 602 | `if (!parse_angle(ptz_payload, angle) \|\| !std::isfinite(angle)) {` |
| `HikvisionHandle::continuous_watchdog_loop` | 625 | `void HikvisionHandle::continuous_watchdog_loop() {` |
| `this_thread::sleep_for` | 627 | `std::this_thread::sleep_for(` |
| `chrono::milliseconds` | 628 | `std::chrono::milliseconds(kContinuousWatchdogPeriodMs));` |
| `steady_clock::now` | 639 | `std::chrono::steady_clock::now() - last_continuous_command_time_);` |
| `HikvisionHandle::hikvision_controller` | 650 | `HikvisionHandle::Result HikvisionHandle::hikvision_controller(` |
| `std::isfinite` | 656 | `if (!std::isfinite(angle) \|\| angle < 0.0) {` |
| `HikvisionHandle::handle_get_picture` | 715 | `// void HikvisionHandle::handle_get_picture() {` |

### src/hikvision/hikvihandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `HikvisionHandle` | 24 | `HikvisionHandle(std::string ip = "192.168.44.64",` |
| `~HikvisionHandle` | 28 | `~HikvisionHandle();` |
| `hikvision_controller` | 30 | `Result hikvision_controller(const std::string& command, double angle);` |
| `continuous_controller` | 31 | `Result continuous_controller(const std::string& command);` |
| `handle_command` | 32 | `json handle_command(const json& data);` |
| `handle_continuous_command` | 33 | `json handle_continuous_command(const json& data);` |
| `handle_get_picture` | 34 | `void handle_get_picture();` |
| `request` | 58 | `HttpResponse request(const std::string& method,` |
| `get_position` | 61 | `bool get_position(Position& position, std::string& error_message) const;` |
| `get_capabilities` | 62 | `bool get_capabilities(Capabilities& capabilities, std::string& error_message) const;` |
| `set_position` | 63 | `bool set_position(const Position& position, std::string& error_message) const;` |
| `set_continuous_velocity` | 64 | `bool set_continuous_velocity(int pan_speed,` |
| `execute_continuous_command` | 68 | `json execute_continuous_command(const std::string& response_type,` |
| `continuous_watchdog_loop` | 71 | `void continuous_watchdog_loop();` |

### src/localWebsocket/LightPolicy.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `selectLightStatus` | 4 | `inline LightStatus selectLightStatus(bool has_bms, bool bms_error, double soc,` |

### src/localWebsocket/LocalWebsocketServer.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `LocalWebsocketServer::LocalWebsocketServer` | 5 | `LocalWebsocketServer::LocalWebsocketServer(std::string ip, int port,` |
| `std::move` | 7 | `: ip_(std::move(ip)), equipment_path_(std::move(equipment_path)),` |
| `std::move` | 8 | `test_path_(std::move(test_path)), port_(port) {` |
| `LocalWebsocketServer::~LocalWebsocketServer` | 47 | `LocalWebsocketServer::~LocalWebsocketServer() { stop(); }` |
| `LocalWebsocketServer::start` | 49 | `bool LocalWebsocketServer::start() {` |
| `ip::make_address` | 58 | `const auto address = boost::asio::ip::make_address(ip_);` |
| `tcp::endpoint` | 59 | `server_.listen(boost::asio::ip::tcp::endpoint(address, static_cast<unsigned short>(port_)));` |
| `std::thread` | 61 | `thread_ = std::thread([this] {` |
| `LocalWebsocketServer::stop` | 79 | `void LocalWebsocketServer::stop() {` |
| `LocalWebsocketServer::setMessageHandler` | 94 | `void LocalWebsocketServer::setMessageHandler(Service service, MessageHandler handler) {` |
| `std::move` | 96 | `(service == Service::Equipment ? ai_handler_ : light_handler_) = std::move(handler);` |
| `LocalWebsocketServer::send` | 99 | `void LocalWebsocketServer::send(Handle connection, const std::string& payload, bool binary) {` |
| `LocalWebsocketServer::broadcast` | 109 | `void LocalWebsocketServer::broadcast(Service service, const std::string& payload, bool binary) {` |

### src/localWebsocket/LocalWebsocketServer.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `LocalWebsocketServer` | 17 | `LocalWebsocketServer(std::string ip, int port, std::string equipment_path,` |
| `~LocalWebsocketServer` | 19 | `~LocalWebsocketServer();` |
| `start` | 20 | `bool start();` |
| `stop` | 21 | `void stop();` |
| `setMessageHandler` | 22 | `void setMessageHandler(Service service, MessageHandler handler);` |
| `send` | 23 | `void send(Handle connection, const std::string& payload, bool binary = false);` |
| `broadcast` | 24 | `void broadcast(Service service, const std::string& payload, bool binary = false);` |

### src/main.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `ros::init` | 32 | `ros::init(argc, argv, "leggedog_gateway_node");` |
| `std::to_string` | 53 | `std::string ws_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_navbridge + "/" + robot_id;` |
| `std::to_string` | 54 | `std::string bms_websocket_url = config.gateway_protocol + "://" + config.gateway_host + ":" + std::to_string(config.gateway_port) + config.endpoints_register + "/" + robot_id;` |
| `ros::WallDuration` | 96 | `ros::WallTimer light_timer = nh.createWallTimer(ros::WallDuration(1.0),` |
| `ros::spin` | 128 | `ros::spin();` |

### src/mapping/maphandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `MappingHandle::MappingHandle` | 21 | `MappingHandle::MappingHandle(std::string device_id,` |
| `std::move` | 35 | `: device_id_(std::move(device_id)),` |
| `std::move` | 36 | `scripts_path_(std::move(scripts_path)),` |
| `std::move` | 37 | `map_dir_(std::move(map_dir)),` |
| `std::move` | 38 | `map_files_(std::move(map_files)),` |
| `std::move` | 42 | `send_to_cloud_(std::move(send_to_cloud)),` |
| `std::move` | 43 | `log_file_access_(std::move(log_file_access)),` |
| `std::move` | 44 | `script_log_command_(std::move(script_log_command)),` |
| `std::move` | 45 | `shell_quote_(std::move(shell_quote)),` |
| `std::move` | 46 | `log_runtime_flags_(std::move(log_runtime_flags)),` |
| `std::move` | 47 | `log_wait_status_(std::move(log_wait_status)),` |
| `std::move` | 48 | `finish_command_(std::move(finish_command)) {}` |
| `MappingHandle::handle_start_mapping` | 50 | `void MappingHandle::handle_start_mapping(const json& data) {` |
| `fs::remove` | 53 | `fs::remove("/home/jianmi/g_build_map/all_raw_points.pcd");` |
| `fs::remove` | 54 | `fs::remove("/home/jianmi/g_build_map/gloabl_map.pcd");` |
| `fs::remove` | 55 | `fs::remove("/home/jianmi/g_build_map/gloabl_map.pickle");` |
| `fs::path` | 84 | `(fs::path(scripts_path_) / "mapping.sh").string();` |
| `std::strerror` | 108 | `errno, std::strerror(errno));` |
| `std::fprintf` | 118 | `std::fprintf(` |
| `std::strerror` | 121 | `mapping_script.c_str(), errno, std::strerror(errno));` |
| `std::exit` | 123 | `std::exit(1);` |
| `std::time` | 139 | `{"timestamp", static_cast<int>(std::time(nullptr))}` |
| `MappingHandle::stop_and_uploadmap_async` | 150 | `void MappingHandle::stop_and_uploadmap_async(json data) {` |
| `this_thread::sleep_for` | 183 | `std::this_thread::sleep_for(std::chrono::seconds(2));` |
| `fs::path` | 187 | `std::string process_script = (fs::path(scripts_path_) / "map_process.sh").string();` |

### src/mapping/maphandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `std::string` | 21 | `using ScriptLogCommand = std::function<std::string(const std::string&, const std::string&)>;` |
| `std::string` | 22 | `using ShellQuote = std::function<std::string(const std::string&)>;` |
| `MappingHandle` | 27 | `MappingHandle(std::string device_id,` |
| `handle_start_mapping` | 42 | `void handle_start_mapping(const json& data);` |
| `stop_and_uploadmap_async` | 43 | `void stop_and_uploadmap_async(json data);` |

### src/modifyValue/modifyHandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `ModifyHandle::ModifyHandle` | 15 | `ModifyHandle::ModifyHandle(tinyxml2::XMLDocument& doc, SendToCloud send_to_cloud,` |
| `std::move` | 18 | `send_to_cloud_(std::move(send_to_cloud)),` |
| `ModifyHandle::~ModifyHandle` | 23 | `ModifyHandle::~ModifyHandle() {}` |
| `std::string` | 228 | `if (std::string(element->Name()) == param_type.c_str()) {` |
| `ModifyHandle::getParam` | 244 | `std::string ModifyHandle::getParam(const std::string& param_type, const std::string& name) {` |
| `ModifyHandle::setParam` | 260 | `bool ModifyHandle::setParam(const std::string& param_type, const std::string& name, const std::string& value) {` |
| `ModifyHandle::setYamlValue` | 276 | `bool ModifyHandle::setYamlValue(YAML::Node root, const std::string& path, const json& value) {` |
| `std::getline` | 284 | `while (std::getline(ss, key, '/'))` |
| `ModifyHandle::save` | 316 | `bool ModifyHandle::save(const std::string& file_path) {` |
| `ModifyHandle::modifyYaml` | 320 | `bool ModifyHandle::modifyYaml(const std::string& path, const json& value) {` |
| `std::getline` | 326 | `while (std::getline(path_stream, key, '/')) {` |
| `std::getline` | 342 | `while (std::getline(fin, line)) {` |
| `std::remove` | 453 | `std::remove(temp_path.c_str());` |
| `std::rename` | 456 | `if (std::rename(temp_path.c_str(), parameter_yaml_path_.c_str()) != 0) {` |
| `std::remove` | 457 | `std::remove(temp_path.c_str());` |
| `ModifyHandle::updateParam` | 469 | `bool ModifyHandle::updateParam(const json& res_json) {` |

### src/modifyValue/modifyHandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `ModifyHandle` | 20 | `explicit ModifyHandle(` |
| `~ModifyHandle` | 26 | `~ModifyHandle();` |
| `getParam` | 28 | `std::string getParam(const std::string& param_type, const std::string& name);` |
| `setParam` | 29 | `bool setParam(const std::string& param, const std::string& name, const std::string& value);` |
| `save` | 30 | `bool save(const std::string& file_path);` |
| `updateParam` | 31 | `bool updateParam(const json& res_json);` |
| `setYamlValue` | 33 | `bool setYamlValue(YAML::Node root, const std::string& path, const json& value);` |
| `modifyYaml` | 34 | `bool modifyYaml(const std::string& path, const json& value);` |

### src/motionControl/motionHandle.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `MotionHandle::MotionHandle` | 5 | `MotionHandle::MotionHandle(ros::NodeHandle& nh)` |
| `std::thread` | 11 | `watchdog_thread_ = std::thread(&MotionHandle::watchdog_loop, this);` |
| `MotionHandle::~MotionHandle` | 14 | `MotionHandle::~MotionHandle()` |
| `MotionHandle::publish_motion_cmd` | 22 | `void MotionHandle::publish_motion_cmd(` |
| `MotionHandle::robot_state_callback` | 40 | `void MotionHandle::robot_state_callback(` |
| `MotionHandle::watchdog_loop` | 52 | `void MotionHandle::watchdog_loop()` |
| `chrono::milliseconds` | 54 | `constexpr auto kWatchdogInterval = std::chrono::milliseconds(100);` |
| `chrono::milliseconds` | 55 | `constexpr auto kWebControlTimeout = std::chrono::milliseconds(kMotionStopTimeoutMs_);` |
| `chrono::milliseconds` | 56 | `constexpr auto kStandPublishInterval = std::chrono::milliseconds(500);` |
| `ros::ok` | 58 | `while (running_.load(std::memory_order_acquire) && ros::ok()) {` |
| `steady_clock::now` | 63 | `const auto now = std::chrono::steady_clock::now();` |
| `this_thread::sleep_for` | 88 | `std::this_thread::sleep_for(kWatchdogInterval);` |
| `MotionHandle::handle_action_control` | 92 | `void MotionHandle::handle_action_control(const json& data)` |
| `std::stod` | 124 | `value = std::stod(text, &parsed_chars);` |
| `std::isfinite` | 134 | `return std::isfinite(value);` |
| `std::abs` | 155 | `std::abs(linear_velocity) < 1e-6 && std::abs(angular_velocity) < 1e-6;` |
| `steady_clock::now` | 164 | `const auto now = std::chrono::steady_clock::now();` |
| `chrono::milliseconds` | 169 | `std::chrono::milliseconds(kPoseCommandDedupMs_)) {` |
| `std::abs` | 183 | `!motion_publish_active_ && std::abs(motion_lin_x_) < 1e-6 &&` |
| `std::abs` | 184 | `std::abs(motion_lin_y_) < 1e-6 && std::abs(motion_ang_z_) < 1e-6;` |
| `steady_clock::now` | 232 | `last_motion_command_time_ = std::chrono::steady_clock::now();` |

### src/motionControl/motionHandle.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `MotionHandle` | 20 | `explicit MotionHandle(ros::NodeHandle& nh);` |
| `~MotionHandle` | 21 | `~MotionHandle();` |
| `MotionHandle` | 23 | `MotionHandle(const MotionHandle&) = delete;` |
| `handle_action_control` | 26 | `void handle_action_control(const json& data);` |
| `publish_motion_cmd` | 29 | `void publish_motion_cmd(const std::string& target_mode,` |
| `robot_state_callback` | 31 | `void robot_state_callback(const robot_gateway::M20V06RobotState::ConstPtr& msg);` |
| `watchdog_loop` | 32 | `void watchdog_loop();` |

### src/robotState/robotState.cc

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `system_clock::now` | 19 | `std::chrono::system_clock::now().time_since_epoch()).count();` |
| `json::array` | 47 | `json active_alarm_levels = json::array();` |
| `json::array` | 65 | `json active_flags = json::array();` |
| `robotState::robotState` | 104 | `robotState::robotState(` |
| `std::move` | 108 | `: device_id_(std::move(device_id)),` |
| `std::move` | 109 | `send_to_cloud_(std::move(send_to_cloud))` |
| `ros::Duration` | 112 | `ros::Duration(0.2), // websocket 实时返回消息频率，5hz` |
| `robotState::~robotState` | 118 | `robotState::~robotState()` |
| `robotState::read_ros_state_info_callback` | 123 | `void robotState::read_ros_state_info_callback(const std_msgs::String::ConstPtr& msg)` |
| `robotState::read_ros_bms_info_callback` | 136 | `void robotState::read_ros_bms_info_callback(const robot_gateway::M20BmsState::ConstPtr& msg)` |
| `robotState::update_navigation_environment` | 150 | `void robotState::update_navigation_environment(` |
| `json::object` | 164 | `map_info_ = json::object();` |
| `robotState::set_navigation_env_active` | 171 | `void robotState::set_navigation_env_active(bool active)` |
| `robotState::clear_navigation_environment` | 181 | `void robotState::clear_navigation_environment()` |
| `json::object` | 186 | `map_info_ = json::object();` |
| `robotState::publish_snapshot` | 192 | `void robotState::publish_snapshot(const std::string& updated_source)` |

### src/robotState/robotState.h

| 方法/函数 | 行 | 起始代码 |
| --- | --- | --- |
| `robotState` | 18 | `robotState(` |
| `~robotState` | 23 | `~robotState();` |
| `read_ros_state_info_callback` | 25 | `void read_ros_state_info_callback(const std_msgs::String::ConstPtr& msg);` |
| `read_ros_bms_info_callback` | 26 | `void read_ros_bms_info_callback(const robot_gateway::M20BmsState::ConstPtr& msg);` |
| `update_navigation_environment` | 27 | `void update_navigation_environment(` |
| `set_navigation_env_active` | 31 | `void set_navigation_env_active(bool active);` |
| `clear_navigation_environment` | 32 | `void clear_navigation_environment();` |
| `publish_snapshot` | 35 | `void publish_snapshot(const std::string& updated_source);` |
| `json::object` | 43 | `nlohmann::json map_info_ = nlohmann::json::object();` |

## 外部调用脚本原文

以下脚本是建图/导航的实际调用链；其中 rm/pkill/roslaunch 等仅作为源码展示，本次没有执行。GNOME 会话和各绝对路径依赖见部署文档。

### mapping.sh

`/home/jianmi/catkin_pct/Scripts/mapping.sh`

```bash
#!/bin/bash
date
cd /home/jianmi/catkin_pct/Scripts

# source ROS根目录
source /opt/ros/noetic/setup.bash

# 开启slam
./slam.sh
sleep 1


echo "All programs have been activated"
sleep 5
```

### slam.sh

`/home/jianmi/catkin_pct/Scripts/slam.sh`

```bash
#!/bin/bash

# source ROS根目录
source /opt/ros/noetic/setup.bash 

# source 指定工作空间
source /home/jianmi/fast_wa/devel/setup.bash


#1、删除之前的旧图
TARGET_DIR="/home/jianmi/fast_wa/src/FAST-LIVO2/Log/PCD"

#------检查目录是否存在--------
if [ -d "$TARGET_DIR" ]; then
    echo "正在清理目录: $TARGET_DIR"
    # 进入目录并删除所有文件（包含隐藏文件请用 ./*）
    rm -rf "${TARGET_DIR:?}"/*
else
    echo "错误: 目录 $TARGET_DIR 不存在。"
fi

#2、启动 slam
#gnome-terminal -- bash -c "cd;roslaunch fast_lio mapping_c16.launch; exec bash"
roslaunch fast_livo mapping_avia.launch
sleep 1
```

### navigation.sh

`/home/jianmi/catkin_pct/Scripts/navigation.sh`

```bash
#!/bin/bash
date

# source ROS根目录
source /opt/ros/noetic/setup.bash 


# 启动重定位
gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash;source /home/jianmi/fast_wa/devel/setup.bash;roslaunch fast_lio_localization localization_MID360.launch map:='/home/jianmi/g_nav_map/gloabl_map.pcd'"
sleep 5  


#tf pub
#gnome-terminal -- bash -c "python3 /home/jianmi/fast_wa/src/FAST_LIO_LOCALIZATION/scripts/base_link.py"
#sleep 3

#开启导航
gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash;source /home/jianmi/catkin_pct/devel/setup.bash;roslaunch pct_planner global_planner.launch"
sleep 5  

#开启局部路径规划程序
#gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash;python3 /home/jianmi/PCT_planner/run/Neupan/flat_neupan_map.py "
#sleep 3  


#开启楼梯点云滤除程序
#gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash;python3 /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/scripts/clear_stair.py "
#sleep 3  

#开启neupan局部路径规划程序
#gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash;python3 /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/scripts/flat_stair_neupan.py "
#sleep 3   

#开启scan-planner局部路径规划程序
gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash; source /home/jianmi/catkin_scan/devel/setup.bash; roslaunch scan_planner run.launch"
sleep 3 

#开启scan-planner局部路径规划程序
gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash; source /home/jianmi/catkin_scan/devel/setup.bash; cd /home/jianmi/catkin_scan/src/SCAN-Planner-main/src/planner/plan_manage/launch; rviz -d default.rviz"
sleep 3 
echo "所有流程已完成！"
```

### map_process.sh

`/home/jianmi/catkin_pct/Scripts/map_process.sh`

```bash
#!/bin/bash

#1、拷贝原始点云 ---
#删除之前的旧图
TARGET_DIR="/home/jianmi/g_build_map/PCD"

#------检查目录是否存在--------
if [ -d "$TARGET_DIR" ]; then
    echo "正在清理目录: $TARGET_DIR"
    # 进入目录并删除所有文件（包含隐藏文件请用 ./*）
    rm -rf "${TARGET_DIR:?}"/*
else
    echo "错误: 目录 $TARGET_DIR 不存在。"
fi
echo "正在拷贝点云文件..."
# 使用 sudo 如果需要，若已设置免密可直接运行
sudo cp -r /home/jianmi/fast_wa/src/FAST-LIVO2/Log/PCD/* /home/jianmi/g_build_map/PCD/
sleep 1

#2、合并子图
#给权限
sudo chmod -R 777 /home/jianmi/g_build_map/PCD
#合并后生成
python3 /home/jianmi/g_build_map/merge_pcd_with_poses.py
sleep 1

#3、降采样
python3 /home/jianmi/g_build_map/downsample_tool.py
sleep 1

#4、构建代价地图
python3 /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomography/scripts/only_tomograph.py
sleep 1

echo "所有流程已完成！地图已保存在 /home/jianmi/g_build_map下"
```

### map_process2.sh

`/home/jianmi/catkin_pct/Scripts/map_process2.sh`

```bash
#!/bin/bash
date

#构建静态障碍物地图.npy文件
python3 /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/pcd_process.py

#构建navmap的代价地图
python3 /home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomography/scripts/navmap_tomograph.py
```

### kill_all.sh

`/home/jianmi/catkin_pct/Scripts/kill_all.sh`

```bash
#!/bin/bash
date

# source ROS根目录
source /opt/ros/noetic/setup.bash 

# source 指定工作空间
echo "正在使用 rosnode kill 杀掉特定节点..."
rosnode kill /base_link_to_body \
             /global_localization \
             /laserMapping \
             /localPlanner \
             /map_publishe \
             /navActionServer \
             /odom_to_base_link_tf_publisher_tf2 \
             /pointcloud_to_laserscan \
             /taskRvizServer \
             /transform_fusion \
             /livox_2d_node \
             /globalPlanner \
             /base_link_to_body \
             /map_server \
             /closed_loop_controller \
             /go2_gait_publisher   \
             /go2_robot_state_publisher \
             /odom_visualization   \
             /scan_planner_node   \
             > /dev/null 2>&1

# 4. 暴力补刀：针对那些杀不掉或带随机后缀的进程
# 使用进程关键字直接从系统层面抹除
echo "正在清理残留的二进制进程..."
pkill -9 -f /global_localization
pkill -9 -f /laserMapping
pkill -9 -f /localPlanner
pkill -9 -f /map_publishe
pkill -9 -f /navActionServer
#pkill -9 -f /odom_to_base_link_tf_publisher_tf2
pkill -9 -f pointcloud_to_laserscan
pkill -9 -f livox_2d_node
pkill -9 -f rviz
pkill -9 -f /taskRvizServer
pkill -9 -f /transform_fusion
pkill -9 -f /map_server
# 使用脚本文件名进行匹配
pkill -9 -f base_link.py
pkill -9 -f flat_stair_neupan.py
pkill -9 -f clear_stair.py
pkill -9 -f globalPlanner
pkill -9 -f globalPlanner.py
pkill -9 -f localPlanner.py
pkill -9 -f navActionServer.py
pkill -9 -f taskRvizServer.py
pkill -9 -f tomography.py

pkill -9 -f global_localization.py
pkill -9 -f transform_fusion.py
pkill -9 -f base_link_to_body

pkill -9 -f closed_loop_controller
pkill -9 -f go2_gait_publisher  
pkill -9 -f go2_robot_state_publisher
pkill -9 -f odom_visualization  
pkill -9 -f scan_planner_node  

# 5. 最后一步：清理 ROS Master 注册表缓存
# 这一步会让 rosnode list 里的名字彻底消失
echo "正在清理 rosnode cleanup 列表..."
rosnode cleanup <<EOF
y
EOF

echo "清理完成！现在的 rosnode list 应该干净了。"
rosnode list
sleep 1
```

## 源码快照文件清单与 SHA-256

快照只包含本次选读的源码/配置/脚本，不包含完整 build、地图、模型或全部第三方依赖；不能直接当作可编译发布包。SHA-256 用于确认本文依据的内容。

| 相对快照路径 | 字节 | SHA-256 |
| --- | --- | --- |
| `CMakeLists.txt` | 1609 | `a37e9b2daf01021e77cdf7a5b175be6bcfac054d3790c57888b7c3454b38802e` |
| `FSM_DESIGN.md` | 2680 | `9f49ea5b6993e91bdcd64860339c282bc3b9a84668e5b0b1acde3522745264db` |
| `LIGHT_CONTROL.md` | 1874 | `dad80dda3ee84bbed7f89e84df74af9c2424c26fa46d99ac720f8480762268a8` |
| `LOCAL_WEBSOCKET.md` | 2496 | `f71b970294a6708546219766d09ed0eb1cbc1c51168738a4d79df718e4083931` |
| `MOTION_WEB_CONTROL_PROTOCOL.md` | 3841 | `b795f03e1463f87bec2c19db4981b2ade9a8220d9e2f5da18c0ab2c9c445b813` |
| `advanced_param.xml` | 11684 | `ae03ee3db05df663b061d06a6eb9c304db91baacde266f521dfaf25060b73c81` |
| `hikvision_ptz.py` | 10209 | `428a19a38d0ad74b581ed5fc3abc848a6e79ec78db1f96502237c5e9606b54f6` |
| `home/jianmi/catkin_pct/Scripts/kill_all.sh` | 2197 | `97be142b3adb4af37d81d0eb4baaa86c920dcc19d8bfc0ee902a5249a0fb8b88` |
| `home/jianmi/catkin_pct/Scripts/map_process.sh` | 1033 | `10f04b5ce18db842b8d04519602a23e894ec5cef5e38ae3c9913b04ac82f9f4f` |
| `home/jianmi/catkin_pct/Scripts/map_process2.sh` | 288 | `4dadb90f1c111bff6dcad8656119d3f45cef493ef05ec6726f58f9c2812902d2` |
| `home/jianmi/catkin_pct/Scripts/mapping.sh` | 191 | `031a26b871e19eafb26f3332ef30fb1c1a34a7b1ac7b0ac4a1ca0d81fd215bb7` |
| `home/jianmi/catkin_pct/Scripts/navigation.sh` | 1705 | `71454a6cb1d8cf2154c43132c02d1f291bcb4bf5f917f09aa69cddf0481faa93` |
| `home/jianmi/catkin_pct/Scripts/slam.sh` | 655 | `c18114ccca7065ca60324d252778257dab63dd7e3cceba024eca4ef88d7ec934` |
| `home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/launch/global_planner.launch` | 1101 | `d351f92f8a515b6eed89f1486a2a6743d1864848a5a00fe608962b8fc2083468` |
| `home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/config/params.yaml` | 2711 | `84fc3d600bf5790b046e4863037b2e3a5862f606b18fb8dbd54367d3d78e72c6` |
| `home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/scripts/navActionServer.py` | 27830 | `cd69820f11b544955aceff00942001a1d2c0a32518a71d6599f10691ddeef8c9` |
| `home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/scripts/taskRvizServer.py` | 18680 | `30ea9858ec2bdba3e5353f37a31edaeac38062a8e97bcb4347ab1843d0037c5c` |
| `home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/scripts/tomography.py` | 10435 | `ef942f8569fab2b3b7603205fb336a2c98892634bc0d9a7d6cf29a4028bd13f3` |
| `home/jianmi/catkin_scan/src/SCAN-Planner-main/src/planner/plan_manage/launch/advanced_param.xml` | 8622 | `ce3e07d394cc1ad2c061168d3753cfbb6099fe793231da3eb64849e972a576b0` |
| `home/jianmi/catkin_scan/src/SCAN-Planner-main/src/planner/plan_manage/launch/run.launch` | 3063 | `933741200e0b3069bd90cdef2ec6e37476daef3d05538b73848bd1b51e49b838` |
| `home/jianmi/fast_wa/src/FAST_LIO_LOCALIZATION/launch/localization_MID360.launch` | 1972 | `fe541bf15ad8fa52e6ca5ea307dc5600ee042b402684ee117e38407e6efbcc5d` |
| `home/jianmi/fast_wa/src/FAST_LIO_LOCALIZATION/scripts/global_localization.py` | 13486 | `ee90efdf04aa76346a0b4cacdee8ef4256180e00acb94ccc8e8faa2a9ef17b6c` |
| `home/jianmi/fast_wa/src/FAST_LIO_LOCALIZATION/scripts/publish_initial_pose.py` | 1251 | `854cf7f634f5b5b5e37350ee57f07f4b5d8e1f48e762ccb6fc5323bc899f3142` |
| `home/jianmi/fast_wa/src/FAST_LIO_LOCALIZATION/scripts/transform_fusion.py` | 2898 | `83c7a190e44a590bb672becc8915faafd41b916a35c3bab0aa0cad459738999b` |
| `home/jianmi/grebotMap_jm/build/robot.yaml` | 681 | `c7d7aa98458c86d7432421dc29386c15eefa0ea521a63bb4c4a3a01fa7c5d926` |
| `include/CommandExecutor.h` | 10958 | `42058feae38ae1c689d7047125ca61d411ff8059871e93ce3df7607640969322` |
| `include/RobotFsm.h` | 6132 | `af144c1caaa3d9dde6bb87534c9d50079005c8e8fcb710aedd0cbaccabd1843c` |
| `include/common/cmdline.h` | 18735 | `bfdc2300b490cc5b9f4a591681e1554044bc36da084f434caea9fbda6af4b8fd` |
| `include/common/json.hpp` | 953436 | `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63` |
| `include/common/log_manager.h` | 4337 | `1714d48df00ec592af492b4eff0f0e9c941c07da7d146b7c71e811327526a779` |
| `include/common/qt_example.h` | 170862 | `26f888e3744fa2718cb877ab16050ac8b301de72eb46a27b3cb9085dd026db67` |
| `include/common/utils.h` | 5900 | `091e37b9e25c79fc66d9c1efc80c81af362f1a248a1b61122191d5aa22f39bd8` |
| `include/common/version.h` | 170 | `fccbbd1c0415eccea0b8cfee8fdad101cafb56fee2a21378749a5fce9896d89d` |
| `include/config/yamlConfig.h` | 2639 | `e4ead7d8f3a08dbbd9474fa5ad6f4803db39c34136bb04a642f436e780b08056` |
| `include/http_map_uploader.h` | 23757 | `d5dfd10f03adf4f65fd9681beebde6b7c50257eef67875ce27eb42ebec19eece` |
| `include/manager.h` | 7793 | `b716e06601aafd7358a4e7e74d64e31cc8f241a02b51d8ba39ea5c61b8545685` |
| `params.yaml` | 5974 | `d4ab91165f1ed1fc1444537c8aa85458391c0f8f571199cc06cad0aa6d74daff` |
| `params22.yaml` | 5975 | `a755c2bb3bbfe2c988e23e2dfeb822d6d2e688e22dddc586f069ccd3482c48a7` |
| `readme.md` | 1206 | `73656bac360c22e92dee0cadacdbc2494f9b4837c9421ab7f78eaeb23a1d190a` |
| `robot.yaml` | 514 | `ace4ee097095994879b5f960b20efff1ea43e85fe00117bb6d181a6bfcee4e3e` |
| `src/CommandExecutor.cc` | 107134 | `aa87de303e8a0ade3ab3133f30260de323da2be2f817520701757cc425f6cf67` |
| `src/aiRecognition/AiRecognitionAckHandler.cc` | 5045 | `99bd387abf2927a1af557553749be7e164792785ef497415e041496cd5b5c35b` |
| `src/aiRecognition/AiRecognitionAckHandler.h` | 437 | `8bcffb9675a331099c273e54de275a2815f64e6a2767c67a01445490aead65f4` |
| `src/bms/BmsForwarder.cc` | 27188 | `4359209c59d3d3b7431767edbbe48adb96d06dbe8ba3fbe477b0e65dbf2448eb` |
| `src/bms/BmsForwarder.h` | 672 | `51d28ab9b3615cdfb6f3fbbc6f87faf1e2f315d15591b12b9bc5a24038acd991` |
| `src/bms/CMakeLists.txt` | 593 | `57f8ca24bb0de0923e9e20096691758c5f5d1723b1dcc2de96be3518a92abb60` |
| `src/broadcast/CMakeLists.txt` | 131 | `398cdecdcb19e04fd419d5677420065357cd7e97ab798253d3a7356511c93c95` |
| `src/broadcast/broadcastHandle.cc` | 1351 | `bbffa1ae14abaed670539dda0dba10784ff746d30f621d5362febb7fd22dd871` |
| `src/broadcast/broadcastHandle.h` | 554 | `3297f6f8a7cdfcba941174682e9d592648e73ebcd0f62691105cbaf035f50d1e` |
| `src/follow/CMakeLists.txt` | 107 | `ff3b4cc96644b519c8b37d7c1890847589319db934ce6d65d72aa38f6c94cdde` |
| `src/follow/followHandle.cc` | 5161 | `ea7efbc816392efe5386b3c407e207ca05a7ce34be7d01c94d625df511cc3fbf` |
| `src/follow/followHandle.h` | 513 | `7167b41fc33d58543ffa702c6c86e7f625df8e6bde636684002e1a5295972b09` |
| `src/gateway/CMakeLists.txt` | 122 | `c08d0b2e79badbf868222e25574584ff42165cf5ab5cbe45e7a0cfaf0c7a81b7` |
| `src/gateway/gateway.cc` | 40692 | `b7fa4a269e35ce01747ee9cc747538a5e83f1b2d2d3f947a8a3e37f4b4c93544` |
| `src/gateway/gateway.h` | 5379 | `be98667ac8ba3ad598cff5c612a27ca0cdea80d782812f26d48c65d47a219173` |
| `src/hikvision/CMakeLists.txt` | 342 | `84baf4bcd9d24fc88b76dbaf1fdfad0ed42a2b9b6c161e3af7e1796f6c57bc24` |
| `src/hikvision/hikvihandle.cc` | 23017 | `04cd6d5ef5301bdd80506fa63a42d3a65af2c6832924ac36ab24b938a784e9bd` |
| `src/hikvision/hikvihandle.h` | 2711 | `596a20165df68d9116d37c7e4cbcf06629860fa1215e16188936cc7fdd5f35c5` |
| `src/localWebsocket/LightPolicy.h` | 987 | `e60ee4e2da5dc2ac7b52270434d48e097920d8e476227219298065dbb5146191` |
| `src/localWebsocket/LocalWebsocketServer.cc` | 5413 | `c89a3afef2cdda8a2ebf46ea150eb67e25338bf37537ebf85c8595a245f7c17d` |
| `src/localWebsocket/LocalWebsocketServer.h` | 1216 | `400440b07dcf53097d6332a1fa76410c982392595dcee7c08920dcc9267c8e93` |
| `src/main.cc` | 6048 | `3d95425c9f3534b0bfe965670726430e1c9b684e2c13a9d3780ab2cc572f8dcf` |
| `src/mapping/CMakeLists.txt` | 128 | `4a4874a34377ddc98dd916d9e60ab736d795c56ca6f32e341e6ecaa56ba375db` |
| `src/mapping/maphandle.cc` | 10965 | `e56020e089b38cbd3d8864c892c7e3748209d6c23ed009d4ca267b0fa8ee47a4` |
| `src/mapping/maphandle.h` | 2016 | `6b5d19d0332806c0503860e85ac77773ec00ede7cbb83dbe6e11965418b61407` |
| `src/modifyValue/CMakeLists.txt` | 135 | `d328a80ad9810810cedd697efe9e362b650228120b7c7bcb8351a758d08aecae` |
| `src/modifyValue/modifyHandle.cc` | 17134 | `f4339e9d6c7b3f99eb7fdbe1aaa0a84771118251efb3b712be943bd42958d62c` |
| `src/modifyValue/modifyHandle.h` | 1215 | `84dcea5cba284c2f70f4fbc08b525a4b71936ab5c5aa30700c19606b5e2dc873` |
| `src/motionControl/CMakeLists.txt` | 140 | `2fe8c339b6a4cbb82d550f593e1c83ae8ddf89f17827b660cafe930c385f2a75` |
| `src/motionControl/motionHandle.cc` | 9762 | `6375e3e1d2991dc6a5ce851896a7e1f0db6eb6c37fe4587ab3a202dec3d4a0e5` |
| `src/motionControl/motionHandle.h` | 1635 | `5de1aede943cbbc6fa07f1f39d4332e8d3dff73f517c5adc7bafeec9c714510c` |
| `src/robotDto.h` | 1473 | `36fd64b503a3b7e53163de9ec15e2295865cb196b5ad51b6ece0fbcac4f36819` |
| `src/robotState/CMakeLists.txt` | 319 | `f5952ed55e3c5655d8c5b866d72bab97398e4c8ce511ab75b79e0bf4bb2edcdb` |
| `src/robotState/robotState.cc` | 8655 | `8c6f3fd72b0b25a5d2d7f5e41bab439f3a4f176b7e4dbf71db7f173b3f672a36` |
| `src/robotState/robotState.h` | 1255 | `f53327ed8aa68de835f2f1a80b327b2aff55c344608df34da0b1adebe3e77e8e` |
| `tests/ai_ack_handler_test.cc` | 1281 | `fcb63e592f902704a072820f937142f57a62dcae9d89fdac81ad83f2fb070106` |
| `tests/follow_http_mock_test.py` | 2322 | `64e237cb26f88ff732028bbfd76532253a96a5a81e2ccbb4ede9f6177bc2b906` |
| `tests/follow_http_test.cc` | 2047 | `124f8dab080f7155ace7b223917ffe95329175a5b2f3688d2195e8bbdf6fe10a` |
| `tests/light_policy_test.cc` | 1182 | `ca9b40b143ba69135f8a176d38625b59d9e20ff1eda6dfde362211e5eb873668` |
| `tests/local_websocket_smoke.cc` | 799 | `aa377d305d86ab196ba4dd4faf5c988570eff7b66acf9b8e8aab50e23c743d43` |
| `tests/local_websocket_smoke.py` | 2781 | `72af6830d3a7cf886a4ad02469d49ca0eb5f447b8e4a5c33a4ceffbcc80b5b10` |
| `tests/robot_fsm_test.cc` | 2498 | `7db08a62464fbd1ab726725202a9cf93a0216dc7f9c9a3d0aaee401592ca9882` |