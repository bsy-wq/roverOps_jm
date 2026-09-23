/*

 The feature was designed on September 17, 2026, to modify YAML and XML files using parameters passed via WebSocket.   

*/


#include "modifyHandle.h"
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace tinyxml2;

ModifyHandle::ModifyHandle(tinyxml2::XMLDocument& doc, SendToCloud send_to_cloud,
                    const std::string& device_id, const std::string& parameter_path, const std::string& parameter_yaml_path)
    : doc_(doc),
      send_to_cloud_(std::move(send_to_cloud)),
      device_id_(device_id),
      parameter_path_(parameter_path),
      parameter_yaml_path_(parameter_yaml_path) {}

ModifyHandle::~ModifyHandle() {}

bool checkType(const json& value, const std::string& expected_type) {
    if (expected_type == "string") {
        return value.is_string();
    }

    if (expected_type == "bool") {
        return value.is_boolean();
    }

    if (expected_type == "int") {
        return value.is_number_integer();
    }

    if (expected_type == "double") {
        return value.is_number_float();
    }

    return false;
}

std::unordered_map<std::string, std::string> param_types = {

    {"max_vel", "double"},
    {"max_acc", "double"},
    {"planning_horizon", "double"},

    {"grid_map/sensor_type", "string"},
    {"grid_map/cloud_is_world", "bool"},
    {"grid_map/need_extrinsic", "bool"},

    {"fsm/navi_mode", "int"},
    {"fsm/thresh_replan", "double"},
    {"fsm/thresh_no_replan", "double"},
    {"fsm/planning_horizon", "double"},
    {"fsm/emergency_time_", "double"},
    {"fsm/fail_safe", "bool"},
    {"fsm/max_replan_fail_count", "int"},
    {"fsm/is_real_world", "bool"},

    {"fsm/enable_rotation_check", "bool"},
    {"fsm/rot_check_len_front", "double"},
    {"fsm/rot_check_len_back", "double"},
    {"fsm/rot_check_half_width", "double"},
    {"fsm/rot_check_max_sweep", "double"},

    {"fsm/redline_crash_dist", "double"},
    {"fsm/redline_half_width", "double"},
    {"fsm/redline_step", "double"},

    {"grid_map/resolution", "double"},
    {"grid_map/sliding_map_size_x", "double"},
    {"grid_map/sliding_map_size_y", "double"},
    {"grid_map/sliding_map_size_z", "double"},
    {"grid_map/map_sliding_thresh", "double"},
    {"grid_map/double_cylinder_radius", "double"},
    {"grid_map/double_cylinder_offset", "double"},
    {"grid_map/body_height", "double"},
    {"grid_map/obstacles_inflation_z_up", "double"},
    {"grid_map/obstacles_inflation_z_down", "double"},

    {"grid_map/cx", "double"},
    {"grid_map/cy", "double"},
    {"grid_map/fx", "double"},
    {"grid_map/fy", "double"},

    {"grid_map/depth_filter_maxdist", "double"},
    {"grid_map/depth_filter_mindist", "double"},
    {"grid_map/depth_filter_margin", "int"},
    {"grid_map/k_depth_scaling_factor", "double"},
    {"grid_map/skip_pixel", "int"},

    {"grid_map/p_hit", "double"},
    {"grid_map/p_miss", "double"},
    {"grid_map/p_min", "double"},
    {"grid_map/p_max", "double"},
    {"grid_map/p_occ", "double"},
    {"grid_map/max_ray_length", "double"},
    {"grid_map/vis_height", "double"},

    {"grid_map/decay_enable", "bool"},
    {"grid_map/decay_time_thresh", "double"},
    {"grid_map/decay_rate", "double"},

    {"manager/max_vel", "double"},
    {"manager/max_acc", "double"},
    {"manager/max_jerk", "double"},
    {"manager/control_points_distance", "double"},
    {"manager/feasibility_tolerance", "double"},
    {"manager/planning_horizon", "double"},

    {"optimization/lambda_smooth", "double"},
    {"optimization/lambda_collision", "double"},
    {"optimization/lambda_feasibility", "double"},
    {"optimization/lambda_fitness", "double"},
    {"optimization/dist0", "double"},
    {"optimization/enable_narrow_adaptation", "bool"},
    {"optimization/max_vel", "double"},
    {"optimization/vel_tolerance", "double"},
    {"optimization/max_acc", "double"},
    {"optimization/acc_tolerance", "double"},

    {"closed_loop_controller/time_forward", "double"},
    {"closed_loop_controller/heading_error_threshold", "double"},
    {"closed_loop_controller/kp_pos", "double"},
    {"closed_loop_controller/kp_yaw", "double"},
    {"closed_loop_controller/max_vx", "double"},
    {"closed_loop_controller/max_vy", "double"},
    {"closed_loop_controller/max_vyaw", "double"},
    {"closed_loop_controller/finish_dist", "double"},

    {"closed_loop_controller/box_front", "double"},
    {"closed_loop_controller/box_back", "double"},
    {"closed_loop_controller/box_half_width", "double"},
    {"closed_loop_controller/box_t_step", "double"},

    {"closed_loop_controller/enable_y_velocity", "bool"},

    {"closed_loop_controller/deadzone_vx", "double"},
    {"closed_loop_controller/deadzone_vy", "double"},
    {"closed_loop_controller/deadzone_vyaw", "double"},

    {"closed_loop_controller/robot_max_vx", "double"},
    {"closed_loop_controller/robot_max_vy", "double"},
    {"closed_loop_controller/robot_max_vyaw", "double"},

    {"closed_loop_controller/brake_back_duration", "double"},
    {"closed_loop_controller/brake_back_vx", "double"},
    {"closed_loop_controller/stair_max_vyaw", "double"},
    {"closed_loop_controller/stair_max_vx", "double"}
};

std::unordered_map<std::string, std::string> param_schema = {

    {"scene", "string"},

    {"ros/odom_topic", "string"},
    {"ros/goal_topic", "string"},
    {"ros/path_topic", "string"},
    {"ros/dock_topic", "string"},

    {"ros/map_frame", "string"},
    {"ros/base_frame", "string"},

    {"ros/pointcloud_topic", "string"},
    {"ros/layer_G_topic", "string"},
    {"ros/layer_C_topic", "string"},
    {"ros/tomogram_topic", "string"},

    {"pct_planner/use_quintic", "bool"},
    {"pct_planner/max_heading_rate", "double"},

    {"tomography/resolution", "double"},
    {"tomography/slice_dh", "double"},
    {"tomography/ground_h", "double"},

    {"tomography/kernel_size", "int"},
    {"tomography/interval_min", "double"},
    {"tomography/interval_free", "double"},

    {"tomography/slope_max", "double"},
    {"tomography/step_max", "double"},

    {"tomography/standable_ratio", "double"},
    {"tomography/cost_barrier", "double"},
    {"tomography/safe_margin", "double"},
    {"tomography/inflation", "double"},

    {"nav/timeout", "double"},
    {"nav/default_dwell_time", "double"},

    {"nav/sim_initial_pose/x", "double"},
    {"nav/sim_initial_pose/y", "double"},
    {"nav/sim_initial_pose/z", "double"},
    {"nav/sim_initial_pose/yaw", "double"},

    {"nav/robot_center_offset", "double"},
    {"nav/external_timeout", "double"},

    {"nav/max_vx", "double"},
    {"nav/max_vy", "double"},
    {"nav/max_w", "double"},

    {"nav/switch_distance", "double"},

    {"nav/enable_start_align", "bool"},
    {"nav/align_angle_thresh", "double"},
    {"nav/align_done_thresh", "double"},
    {"nav/align_lookahead_dist", "double"},
    {"nav/align_stable_time", "double"},

    {"nav/precision_dist_thresh", "double"},
    {"nav/arrive_dist_thresh", "double"},
    {"nav/arrive_angle_thresh", "double"},

    {"nav/rdp_epsilon", "double"},
    {"nav/step_z_thresh", "double"},
    {"nav/stair_window_size", "int"},
    {"nav/slope_thresh_deg", "double"},
};

tinyxml2::XMLElement* findParam(const std::string& param_type, tinyxml2::XMLElement* element, const std::string& name) {
    if (!element) { return nullptr; }

    if (std::string(element->Name()) == param_type.c_str()) {
        const char* param_name = element->Attribute("name");
        if(param_name && name == param_name) {
            return element;
        }
    }

    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        tinyxml2::XMLElement* result = findParam(param_type, child, name);

        if (result) { return result; }
    }

    return nullptr;
}

std::string ModifyHandle::getParam(const std::string& param_type, const std::string& name) {
    tinyxml2::XMLElement* root = doc_.FirstChildElement("launch");

    tinyxml2::XMLElement* param = findParam(param_type, root, name);
    if(!param) { return ""; }

    const char* value;
    if (param_type == "param") {
        value = param->Attribute("value");
    } else if (param_type == "arg") {
        value = param->Attribute("default");
    }
    
    return value ? value : "";
}

bool ModifyHandle::setParam(const std::string& param_type, const std::string& name, const std::string& value) {
    tinyxml2::XMLElement* root = doc_.FirstChildElement("launch");
    tinyxml2::XMLElement* param = findParam(param_type, root, name);
    if(!param) { 
        std::cout << "找不到参数: " << std::endl;
        return false;
    }
    if (param_type == "param") {
        param->SetAttribute("value", value.c_str());
    } else if (param_type == "arg") {
        param->SetAttribute("default", value.c_str());
    }
    
    return true;
}

bool ModifyHandle::setYamlValue(YAML::Node root, const std::string& path, const json& value) {
    YAML::Node node = root;

    std::stringstream ss(path);
    std::string key;

    std::vector<std::string> keys;

    while (std::getline(ss, key, '/'))
    {
        if (!key.empty()) {
            keys.push_back(key);
        }
    }

    if (key.empty()) { return false; }

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (!node[keys[i]]) return false;
        node = node[keys[i]];
    }

    const std::string& last_key = keys.back();

    if (value.is_boolean()) {
        node[last_key] = value.get<bool>();
    } else if (value.is_number_integer()) {
        node[last_key] = value.get<int>();
    } else if (value.is_number_float()) {
        node[last_key] = value.get<double>();
    } else if (value.is_string()) {
        node[last_key] = value.get<std::string>();
    } else {
        return false;
    }

    return true;
    
}

bool ModifyHandle::save(const std::string& file_path) {
    return doc_.SaveFile(file_path.c_str()) == tinyxml2::XML_SUCCESS;
}

bool ModifyHandle::modifyYaml(const std::string& path, const json& value) {
    try
    {
        std::vector<std::string> target_keys;
        std::stringstream path_stream(path);
        std::string key;
        while (std::getline(path_stream, key, '/')) {
            if (!key.empty()) {
                target_keys.push_back(key);
            }
        }
        if (target_keys.empty()) {
            return false;
        }

        std::ifstream fin(parameter_yaml_path_);
        if (!fin.is_open()) {
            return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(fin, line)) {
            lines.push_back(line);
        }
        fin.close();

        std::vector<std::pair<size_t, std::string>> yaml_path;
        bool modified = false;
        for (std::string& current_line : lines) {
            const size_t first = current_line.find_first_not_of(" \t");
            if (first == std::string::npos || current_line[first] == '#') {
                continue;
            }

            bool in_single_quote = false;
            bool in_double_quote = false;
            size_t colon = std::string::npos;
            for (size_t i = first; i < current_line.size(); ++i) {
                const char c = current_line[i];
                if (c == '\'' && !in_double_quote) {
                    in_single_quote = !in_single_quote;
                } else if (c == '"' && !in_single_quote &&
                           (i == 0 || current_line[i - 1] != '\\')) {
                    in_double_quote = !in_double_quote;
                } else if (c == ':' && !in_single_quote && !in_double_quote) {
                    colon = i;
                    break;
                }
            }
            if (colon == std::string::npos) {
                continue;
            }

            size_t key_end = colon;
            while (key_end > first &&
                   (current_line[key_end - 1] == ' ' || current_line[key_end - 1] == '\t')) {
                --key_end;
            }
            const std::string current_key = current_line.substr(first, key_end - first);
            const size_t indent = first;
            while (!yaml_path.empty() && yaml_path.back().first >= indent) {
                yaml_path.pop_back();
            }
            yaml_path.emplace_back(indent, current_key);

            if (yaml_path.size() != target_keys.size()) {
                continue;
            }
            bool path_matches = true;
            for (size_t i = 0; i < target_keys.size(); ++i) {
                if (yaml_path[i].second != target_keys[i]) {
                    path_matches = false;
                    break;
                }
            }
            if (!path_matches) {
                continue;
            }

            size_t value_start = colon + 1;
            while (value_start < current_line.size() &&
                   (current_line[value_start] == ' ' || current_line[value_start] == '\t')) {
                ++value_start;
            }

            in_single_quote = false;
            in_double_quote = false;
            size_t comment = std::string::npos;
            for (size_t i = value_start; i < current_line.size(); ++i) {
                const char c = current_line[i];
                if (c == '\'' && !in_double_quote) {
                    in_single_quote = !in_single_quote;
                } else if (c == '"' && !in_single_quote &&
                           (i == 0 || current_line[i - 1] != '\\')) {
                    in_double_quote = !in_double_quote;
                } else if (c == '#' && !in_single_quote && !in_double_quote &&
                           (i == value_start || current_line[i - 1] == ' ' ||
                            current_line[i - 1] == '\t')) {
                    comment = i;
                    break;
                }
            }

            size_t value_end = comment == std::string::npos ? current_line.size() : comment;
            while (value_end > value_start &&
                   (current_line[value_end - 1] == ' ' || current_line[value_end - 1] == '\t')) {
                --value_end;
            }

            const std::string rendered_value = value.dump();
            current_line.replace(value_start, value_end - value_start, rendered_value);
            modified = true;
            break;
        }

        if (!modified) {
            return false;
        }

        const std::string temp_path = parameter_yaml_path_ + ".tmp";
        std::ofstream fout(temp_path, std::ios::trunc);
        if (!fout.is_open()) {
            return false;
        }
        for (size_t i = 0; i < lines.size(); ++i) {
            fout << lines[i];
            if (i + 1 < lines.size()) {
                fout << '\n';
            }
        }
        fout.close();
        if (!fout) {
            std::remove(temp_path.c_str());
            return false;
        }
        if (std::rename(temp_path.c_str(), parameter_yaml_path_.c_str()) != 0) {
            std::remove(temp_path.c_str());
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        std::cerr << "修改 YAML 失败... " << e.what() << '\n';
        return false;
    }
}

bool ModifyHandle::updateParam(const json& res_json) {
    if (!res_json.contains("name") || !res_json.contains("value")) { std::cout << "缺少参数... " << std::endl; return false;}

    const std::string name = res_json["name"].get<std::string>();
    const std::string file_type = res_json["file_type"].get<std::string>();
    const json& value = res_json["value"];

    if (file_type == "xml") {
        auto local_type = param_types[name];
        if (!checkType(value, local_type)) {
            send_to_cloud_(json{
                {"type", "set_param"},
                {"status", "failed"},
                {"name", name},
                {"expect_type", local_type},
                {"device_id", device_id_},
                {"messgae", "类型不匹配或字段不存在, 请检查后重新发送... "}
            }.dump());
            std::cout << "类型不匹配或字段不存在... " << std::endl;
            return false;
        }

        if (name == "max_vel" || name == "max_acc" || name == "planning_horizon") {
            setParam("arg", name, value.dump());
        } else {
            setParam("param", name, value.dump());
        }
        
        save(parameter_path_);
    } else if (file_type == "yaml") {
        auto local_type_ = param_schema[name];
        if (!checkType(value, local_type_)) {
            send_to_cloud_(json{
                {"type", "set_param"},
                {"status", "failed"},
                {"name", name},
                {"expect_type", local_type_},
                {"device_id", device_id_},
                {"messgae", "类型不匹配或字段不存在, 请检查后重新发送... "}
            }.dump());
            std::cout << "类型不匹配或字段不存在... " << std::endl;
            return false;
        }    

        modifyYaml(name, value);
    }



    return true;
}                                   

                    
