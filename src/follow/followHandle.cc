#include "followHandle.h"
#include <curl/curl.h>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
size_t append_response(char* data, size_t size, size_t count, void* context)
{
    const size_t bytes = size * count;
    auto& body = *static_cast<std::string*>(context);
    if (bytes > 1024 * 1024 || body.size() > 1024 * 1024 - bytes) {
        return 0;
    }
    try {
        body.append(data, bytes);
    } catch (...) {
        return 0;
    }
    return bytes;
}
}

Follow::Follow(std::string device_id, SendToCloud send_to_cloud, std::string base_url)
    : device_id_(std::move(device_id)),
      send_to_cloud_(std::move(send_to_cloud)),
      base_url_(std::move(base_url))
{
    while (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
}

bool Follow::handle_follow_function(const json& request)
{
    json response = json::object();
    try {
        if (!request.is_object()) {
            throw std::runtime_error("follow request must be a JSON object");
        }
        const std::string command = request.value("cmd", "");
        std::string path;
        bool is_get = false;
        if (command == "start_follow") {
            path = "/follow/start";
        } else if (command == "stop_follow") {
            path = "/follow/stop";
        } else if (command == "get_follow") {
            path = "/follow/status";
            is_get = true;
        } else if (command == "estop_follow") {
            path = "/follow/estop";
        } else {
            throw std::runtime_error("unknown follow cmd: " + command);
        }
        static const CURLcode initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (initialized != CURLE_OK) {
            throw std::runtime_error("HTTP initialization failed");
        }
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
            curl_easy_init(), &curl_easy_cleanup);
        if (!curl) {
            throw std::runtime_error("HTTP client initialization failed");
        }
        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(
            curl_slist_append(nullptr, "Content-Type: application/json"),
            &curl_slist_free_all);
        if (!headers) {
            throw std::runtime_error("HTTP header allocation failed");
        }
        const std::string url = base_url_ + path;
        const std::string body = request.dump();
        std::string reply;
        char error[CURL_ERROR_SIZE] = {};
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, 1000L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 3000L);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROXY, "*");
        curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, append_response);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &reply);
        if (is_get) {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
        } else {
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
                             static_cast<curl_off_t>(body.size()));
        }
        // Do not retry: a timed-out motion request may already have executed.
        const CURLcode result = curl_easy_perform(curl.get());
        if (result != CURLE_OK) {
            throw std::runtime_error(std::string("follow HTTP failed: ") +
                (error[0] ? error : curl_easy_strerror(result)));
        }
        long status = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
        response = json::parse(reply, nullptr, false);
        if (!response.is_object()) {
            response = json::object();
            throw std::runtime_error("follow backend returned invalid JSON; HTTP " +
                                     std::to_string(status));
        }
        if (status < 200 || status >= 300) {
            response["ok"] = false;
            if (!response.contains("msg")) {
                response["msg"] = "follow HTTP status " + std::to_string(status);
            }
            response["http_status"] = status;
        } else if (!response.contains("ok") || !response["ok"].is_boolean()) {
            throw std::runtime_error("follow backend response requires boolean ok");
        }
    } catch (const std::exception& error) {
        response["ok"] = false;
        response["msg"] = error.what();
    }
    if (response.contains("ok") && response["ok"].is_boolean() &&
        response["ok"].get<bool>() && request.is_object() &&
        request.contains("cmd") && request["cmd"].is_string()) {
        const std::string command = request["cmd"].get<std::string>();
        if (command == "start_follow") {
            running_.store(true);
            estopped_.store(false);
        } else if (command == "stop_follow" || command == "estop_follow") {
            running_.store(false);
            estopped_.store(command == "estop_follow");
        }
        if (command == "get_follow" && response.contains("data") &&
            response["data"].is_object()) {
            const auto& state = response["data"];
            if (state.contains("running") && state["running"].is_boolean()) {
                running_.store(state["running"].get<bool>());
            }
            if (state.contains("state") && state["state"].is_string()) {
                const std::string name = state["state"].get<std::string>();
                if (name == "estop" || name == "estopped") {
                    running_.store(false);
                    estopped_.store(true);
                } else if (name == "idle" || name == "stopped") {
                    running_.store(false);
                    estopped_.store(false);
                } else if (name == "searching" || name == "following") {
                    running_.store(true);
                    estopped_.store(false);
                }
            }
        }
    }
    // Preserve request metadata for Web response correlation.
    response["type"] = "follow";
    response["device_id"] = device_id_;
    if (request.is_object()) {
        for (const char* key : {"device_id", "timestamp", "seq", "source", "cmd"}) {
            if (request.contains(key)) {
                response[key] = request[key];
            }
        }
    }
    send_to_cloud_(response.dump());
    return response.contains("ok") && response["ok"].is_boolean() &&
           response["ok"].get<bool>();
}
