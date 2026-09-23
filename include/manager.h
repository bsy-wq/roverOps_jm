#pragma once

#include "common/httplib.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include "common/json.hpp"
#include <string>
#include "http_map_uploader.h"
#include "common/utils.h"
#include <utility>
#include <mutex>
#include <functional>

namespace fs = std::filesystem;
using json = nlohmann::json;

using ProgressCallback = std::function<void(const std::string&)>;

class MapManager {
public:
    MapManager(const std::string& http_ip, const int& http_port)
            : http_ip_(http_ip), api_port_(http_port)
    {}

    void set_progress_callback(ProgressCallback cb) {
        progress_callback_ = std::move(cb);
    }
    
    // 从 /home/cat/nav_map 读取, 上传服务器, 同时在缓存目录备份
    std::pair<std::string, std::string> upload_map(const std::string& device_id, const std::string& address,
            const std::string& display_name, const std::string local_temp_dir,
            const std::vector<std::string>& file_list, bool use_gaopei = true) {

        std::cout << "\n[Upload] 启动: " << address << " -> " << display_name << std::endl;
        json payload = {
            {"type", "upload_map"},
            {"device_id", device_id},
            {"display_name", display_name},
            {"address", address}
        };
        if (use_gaopei) {
            payload["gaopei"] = "gaopei";
        }

        json res;
        if(!http_post("/request_upload", payload, res)) {
            std::cout << "[Upload] API 请求失败\n";
            return {"", ""};
        }

        if(res["status"] == "exists") {
            std::cout << "[Upload] 服务器已存在同名地图\n";
            return {"", ""};
        }

        std::string srv_path = res["storage_path"]; // /home/cgw/map_storage/坚米/笑嘻嘻/6d311f7d-598f-4703-8cca-19a62ef576f7
        std::string rel_path = res["relative_path"]; // 坚米/笑嘻嘻/6d311f7d-598f-4703-8cca-19a62ef576f7
        std::string uuid = res["uuid"];
        last_storage_path_ = srv_path;

        // /home/cat/nav_map  ->  /home/cgw/map_storage/坚米/三楼/b910d669-bca0-4f52-8502-13801a869f75
        HttpMapUploader uploader(http_ip_, api_port_);
        uploader.set_progress_callback(progress_callback_);
        if(uploader.connect()) {
            std::cout << "开始上传从： " << local_temp_dir << " 到： " << rel_path << std::endl;
            if (!uploader.upload_files(local_temp_dir, file_list, rel_path, use_gaopei)) {
                uploader.disconnect();
                return {"", ""};
            }
            uploader.disconnect();
            return {rel_path, uuid};
        }

        return {"", ""};
    }
      
    std::pair<std::string, std::string> upload_map_sh(const std::string& device_id, const std::string& address,
            const std::string& display_name, const std::string local_temp_dir,
            const std::vector<std::string>& file_list, std::string storage_path, std::string uuid, bool use_gaopei = true) {

        std::cout << "\n[Upload] 启动: " << address << " -> " << display_name << std::endl;

        // /home/cat/nav_map  ->  /home/cgw/map_storage/坚米/三楼/b910d669-bca0-4f52-8502-13801a869f75
        HttpMapUploader uploader(http_ip_, api_port_);
        uploader.set_progress_callback(progress_callback_);
        if(uploader.connect()) {
            std::cout << "开始上传从： " << local_temp_dir << " 到： " << storage_path << std::endl;
            if (!uploader.upload_files(local_temp_dir, file_list, storage_path, use_gaopei)) {
                uploader.disconnect();
                return {"", ""};
            }
            uploader.disconnect();
            return {storage_path, uuid};
        }

        return {"", ""};
    }

    std::string get_last_storage_path() const { return last_storage_path_; }

    // 直接从服务器下载到本地
    bool download_and_deploy(const std::string& address, const std::string& display_name,
        const std::vector<std::string>& file_list, const std::string& deploy_dir) {
        json res_download;

        if (!http_get("/get_map_info",{{"address", url_encode(address)},
                    {"display_name", url_encode(display_name)}, {"gaopei", "gaopei"}}, res_download))
        {
            std::cout << "[Error] 获取服务器信息失败\n";
            return false;
        }

        std::string storage_path = res_download["storage_path"];

        HttpMapUploader uploader(http_ip_, api_port_);
        uploader.set_progress_callback(progress_callback_);

        if (!uploader.connect()) {
            std::cout << "[Error] uploader connect failed\n";
            return false;
        }

        std::cout << " 开始下载从： " << storage_path << " 到： " << deploy_dir << std::endl;

        bool ok = uploader.download_files(storage_path, file_list, deploy_dir);
        uploader.disconnect();

        return ok;
    }

    std::string return_http_url(const std::string& address, const std::string& display_name,
        const std::vector<std::string>& file_list, const std::string& deploy_dir) {
        json res_download;

        if (!http_get("/get_map_info",{{"address", url_encode(address)},
                    {"display_name", url_encode(display_name)}, {"gaopei", "gaopei"}}, res_download))
        {
            std::cout << "[Error] 获取服务器信息失败\n";
            return "";
        }

        std::string storage_path = res_download["storage_path"];
        return storage_path;
    }

private:
    bool http_post(const std::string& path, const json& payload, json& result) {
        std::lock_guard<std::mutex> lock(http_mutex_);
        httplib::Client cli(http_ip_, api_port_);
        cli.set_connection_timeout(5, 0);

        auto res = cli.Post(path.c_str(), payload.dump(), "application/json");

        if(!res || res->status != 200) {
            std::cout << "[HTTP] POST failed: " << path << " (status: " << (res ? res->status : -1) << ")\n";
            return false;
        }

        result = json::parse(res->body);
        return true;
    }

    bool http_get(const std::string& path, const std::map<std::string, std::string>& params, json& result) {
        std::lock_guard<std::mutex> lock(http_mutex_);
        httplib::Client cli(http_ip_, api_port_);

        std::string query;
        for(auto& [k, v] : params) {
            if(!query.empty()) query += "&";
            query += k + "=" + v;
        }

        std::string url = path + "?" + query;
        auto res = cli.Get(url.c_str());

        if(!res || res->status != 200) {
            std::cout << "[HTTP] GET failed: " << url << " (status: " << (res ? res->status : -1) << ")\n";
            return false;
        }

        result = json::parse(res->body);
        return true;
    }

    bool deploy(const std::string& src, const std::string& dst) {
        try {
            if(fs::exists(dst)) {
                fs::remove_all(dst);
            }
            fs::create_directories(dst);

            for(auto& p : fs::recursive_directory_iterator(src)) {
                fs::path relative = fs::relative(p.path(), src);
                fs::path target = fs::path(dst) / relative;
                if(p.is_directory()) {
                    fs::create_directories(target);
                } else if(p.is_regular_file()) {
                    fs::copy(p.path(), target, fs::copy_options::overwrite_existing);
                }
            }

            return true;
        }
        catch(const std::exception& e) {
            std::cerr << "[Deploy] Error: " << e.what() << '\n';
            return false;
        }
    }

private:
    std::string http_ip_;
    int api_port_;
    ProgressCallback progress_callback_;

    std::string last_storage_path_;
    std::mutex http_mutex_;
};
