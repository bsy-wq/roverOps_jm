// #include "navhandle.h"




// void NvaManager::handle_start_navigation(const json& data) {

//     std::string display_name = data.value("display_name", "Unnamed Map");
//     std::string address = data.value("address", "");
//     std::string uuid_old = data.value("uuid", "");
//     std::string uuid = get_uuid(address, display_name);
//     if (uuid.empty()) {
//         json msg = {
//             {"type", "ack_start_navigation"},
//             {"timestamp", static_cast<int>(time(nullptr))},
//             {"device_id", device_id_},
//             {"code", 4},
//             {"status", "get uuid failed"},
//             {"address", address},
//             {"display_name", display_name}
//         };
//         send_to_cloud(msg.dump());
//         LOG_ERROR("[NavigationEnv] get uuid failed, address={}, display_name={}", address, display_name);
//         finish_command("start_navigation");
//         return;
//     }

//     // 3步文件操作
//     //(1) 删除相应的文件
//     fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/navmap.pcd");
//     fs::remove("/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomogram/navmap.pickle");

//     //(2) 先进行拷贝到 g_nav_map 文件夹下
//     fs::path file_path = fs::path("/home/jianmi/map_manager_local") / uuid;
//     fs::copy(
//         file_path / "navmap.pcd",
//         "/home/jianmi/g_nav_map/navmap.pcd",
//         fs::copy_options::overwrite_existing
//     );

//     fs::copy(
//         file_path / "navmap.pickle",
//         "/home/jianmi/g_nav_map/navmap.pickle",
//         fs::copy_options::overwrite_existing
//     );   

//     //(3) 在从 g_nav_map 文件夹下拷贝到相应的路径下
//     fs::copy(
//         "/home/jianmi/g_nav_map/navmap.pcd",
//         "/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/pcd/navmap.pcd",
//         fs::copy_options::overwrite_existing
//     );

//     fs::copy(
//         "/home/jianmi/g_nav_map/navmap.pickle",
//         "/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/rsc/tomogram/navmap.pickle",
//         fs::copy_options::overwrite_existing
//     );

//     ROS_INFO("    ======================= 开始导航 (handle_start_navigation) =======================    ");
//     LOG_INFO(" 开启导航(handle_start_navigation) ... ");
//     LOG_INFO("[NavigationEnv] start_navigation begin, payload={}", data.dump());
//     log_runtime_flags("start_navigation_begin");
//     navigation_env_ready_.store(false, std::memory_order_release);
//     localization_ready_.store(false, std::memory_order_release);
//     upload_enable_.store(false, std::memory_order_release);
    
//     // 校验参数，如果没有传地址或地图名，可以报错或使用默认值
//     if (address.empty() || display_name.empty()) {
//         ROS_ERROR("Start navigation failed: 'address' or 'display_name' is missing.");
//         LOG_ERROR("[NavigationEnv] missing map information, address={}, display_name={}",
//                   address, display_name);

//         json msg1 = {
//             {"type", "ack_start_navigation"},
//             {"device_id", device_id_},
//             {"code", 1},
//             {"status", "Missing map information"}
//         };
//         send_to_cloud(msg1.dump());
//         LOG_INFO("开始路径上报: {}", msg1.dump());
//         finish_command("start_navigation");
//         return;
//     }
    
//     ROS_INFO("Preparing map: %s -> %s...", address.c_str(), display_name.c_str());

//     launch_background_task(
//         std::thread(
//             &CommandExecutor::start_navigation_async,
//             this,
//             data,
//             uuid_old,
//             uuid
//         )
//     );
// }

// void NvaManager::start_navigation_async(json data, std::string uuid_old, std::string uuid) {

//     std::string address = data.value("address", "");
//     std::string display_name = data.value("display_name", "");
//     LOG_INFO("[NavigationEnv] async prepare map begin, address={}, display_name={}, map_dir={}",
//              address, display_name, MAP_MANAGER_LOCAL);

//     std::cout << "旧的 uuid " << uuid_old << "   新的 uuid " << uuid << std::endl;
//     std::vector<std::string> download_map_files = {"gloabl_map.pcd", "navmap.pcd", "lightweight_map.npy", "navmap.pickle"};
//     bool success = true;
//     fs::path file_path = fs::path(MAP_MANAGER_LOCAL) / uuid;
//     if (uuid_old != uuid)
//     {
//         if (map_manager_)
//         {
//             success = map_manager_->download_and_deploy(
//                 address,
//                 display_name,
//                 download_map_files,
//                 file_path
//             );
//         }
//         else
//         {
//             success = false;
//             LOG_ERROR("[NavigationEnv] map_manager ptr is null, cannot download map");
//         }
//     }

//     std::cout << "map download result: " << (success ? 1 : 0) << std::endl;

//     if (!success) {
//         ROS_ERROR("Map deployment failed for %s. Navigation aborted.", display_name.c_str());
//         LOG_ERROR("[NavigationEnv] map deployment failed, address={}, display_name={}, map_dir={}",
//                   address, display_name, map_dir_);
//         navigation_env_ready_.store(false, std::memory_order_release);
//         localization_ready_.store(false, std::memory_order_release);
//         upload_enable_.store(false, std::memory_order_release);
//         log_runtime_flags("start_navigation_map_deploy_failed");

//         json msg2 = {
//             {"type", "ack_start_navigation"},
//             {"timestamp", (int)time(nullptr)},
//             {"device_id", device_id_},
//             {"code", 2},
//             {"status", "Map preparation failed"}
//         };
//         send_to_cloud(msg2.dump());
//         LOG_INFO("开始路径上报: {}", msg2.dump());
//         finish_command("start_navigation");
//         return;
//     }

//     ROS_INFO("Map ready. Starting navigation environment scripts...");
//     LOG_INFO("[NavigationEnv] map ready, starting scripts");

//     std::string nav_script = scripts_path_ + "/navigation.sh";
//     std::string nav_command = fmt::format(
//     R"(gnome-terminal -- bash -c "set -e;
//         source /opt/ros/noetic/setup.bash &&
//         source /home/jianmi/catkin_ws/devel/setup.bash &&
//         source /home/jianmi/catkin_livox/devel/setup.bash --extend &&
//         source /home/jianmi/catkin_pct/devel/setup.bash --extend &&
//         source /home/jianmi/fast_wa/devel/setup.bash --extend &&
//         export LD_LIBRARY_PATH=/home/jianmi/PCT_planner/planner/lib/3rdparty/gtsam-4.1.1/install/lib:$LD_LIBRARY_PATH &&
//         export LD_LIBRARY_PATH=/home/jianmi/catkin_pct/src/pct-planner/src/pct_planner_ros/lib/3rdparty/gtsam-4.1.1/install/lib:$LD_LIBRARY_PATH &&
//         bash {} ;
//         exec bash"
//         )",nav_script
//     );

//     LOG_INFO("[NavigationEnv] script paths: nav_script={}", nav_script);

//     pid_t nav_pid = fork();
//     if (nav_pid < 0) {
//         ROS_ERROR("Fork failed for navigation script");
//         LOG_ERROR("[NavigationEnv] fork failed for navigation.sh, errno={}, error={}",
//                   errno, std::strerror(errno));
//         navigation_env_ready_.store(false, std::memory_order_release);
//         localization_ready_.store(false, std::memory_order_release);
//         upload_enable_.store(false, std::memory_order_release);
//         log_runtime_flags("start_navigation_nav_fork_failed");

//         json msg4 = {
//             {"type", "ack_start_navigation"},
//             {"timestamp", (int)time(nullptr)},
//             {"device_id", device_id_},
//             {"code", 3},
//             {"status", "Script Error: fork navigation failed"}
//         };
//         send_to_cloud(msg4.dump());
//         LOG_INFO("开始路径上报: {}", msg4.dump());
//         finish_command("start_navigation");
//         return;
//     }

//     if (nav_pid == 0) {
//         setpgid(0, 0);
//         execl("/bin/bash", "bash", "-c", nav_command.c_str(), (char*)NULL);
//         std::fprintf(stderr, "[NavigationEnv] execl failed for navigation.sh, path=%s, errno=%d, error=%s\n",
//                      nav_script.c_str(), errno, std::strerror(errno));
//         perror("execl nav failed");
//         exit(1);
//     }
//     LOG_INFO("[NavigationEnv] navigation.sh started, pid={}", nav_pid);

//     {
//         std::lock_guard<std::mutex> lock(process_mutex_);
//         nav_process_ = nav_pid;
//     }

//     unsigned long long nav_env_attempt_id = 0;
//     {
//         std::lock_guard<std::mutex> lock(navigation_env_mutex_);
//         navigation_env_pending_ = true;
//         nav_env_attempt_id = ++navigation_env_attempt_id_;
//     }

//     launch_background_task(std::thread([this, nav_env_attempt_id]() {
//         std::this_thread::sleep_for(std::chrono::seconds(kNavigationEnvReadyTimeoutSec_));
//         complete_navigation_env_ready(
//             nav_env_attempt_id,
//             false,
//             "Timeout waiting for Tomogram exported");
//     }));

//     sleep(5);

//     navigation_env_ready_.store(false, std::memory_order_release);
//     localization_ready_.store(false, std::memory_order_release);
//     upload_enable_.store(false, std::memory_order_release);
//     ROS_INFO("Navigation environment started. Waiting for tomogram export before initial_pose.");
//     LOG_INFO("[NavigationEnv] environment started, waiting for Tomogram exported, attempt_id={}", nav_env_attempt_id);
//     log_runtime_flags("start_navigation_started");

//     json msg3 = {
//         {"type", "ack_start_navigation"},
//         {"timestamp", (int)time(nullptr)},
//         {"device_id", device_id_},
//         {"code", 0},
//         {"status", "Map " + display_name + " deployed and environment started, waiting for tomogram export"}
//     };
//     send_to_cloud(msg3.dump());
//     std::cout << "开启导航环境以全部准备就绪... " << std::endl;
//     LOG_INFO("开始路径上报: {}", msg3.dump());

// }