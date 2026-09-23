#pragma once

#include <curl/curl.h>
#include "common/json.hpp"
#include "common/utils.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

class HttpMapUploader {
public:
    using ProgressCallback = std::function<void(const std::string&)>;

    HttpMapUploader(const std::string& host, const int& port)
        : host_(host), port_(port) {
        ensure_curl_global_init();
    }

    void set_progress_callback(ProgressCallback cb) {
        progress_callback_ = std::move(cb);
    }

    bool connect() {
        auto res = perform_simple_request("GET", "/health", nullptr, nullptr);
        if (!res.ok || res.http_code < 200 || res.http_code >= 300) {
            std::cout << "[Warn] HTTP health check failed, continue with file transfer\n";
        }

        std::cout << "[Success] HTTP uploader ready\n";
        return true;
    }

    void disconnect() {
        std::cout << "[Info] HTTP uploader closed\n";
    }

    bool upload_files_parallel(const std::string& local_dir,
                               const std::vector<std::string>& file_list,
                               const std::string& remote_dir, bool gaopei = false) {
        struct UploadJob {
            std::string local_path;
            std::string filename;
            size_t file_size = 0;
        };

        auto start = std::chrono::steady_clock::now();
        size_t total_bytes = 0;
        std::vector<UploadJob> jobs;

        for (const auto& file : file_list) {
            fs::path local = fs::path(local_dir) / file;
            if (!fs::exists(local)) {
                std::cout << "[Warn] skip missing file: " << local.string() << "\n";
                continue;
            }

            const size_t file_size = fs::file_size(local);
            total_bytes += file_size;
            jobs.push_back({local.string(), file, file_size});
        }

        std::atomic<size_t> next_job{0};
        std::atomic<int> success{0};
        std::atomic<int> failed{0};
        constexpr size_t kMaxParallelUploads = 4;
        const size_t worker_count = std::min(kMaxParallelUploads, jobs.size());
        std::vector<std::thread> workers;
        workers.reserve(worker_count);

        std::cout << "[Info] starting " << worker_count
                  << " parallel file upload worker(s)\n";

        for (size_t worker = 0; worker < worker_count; ++worker) {
            workers.emplace_back([&, worker]() {
                while (true) {
                    const size_t index = next_job.fetch_add(1, std::memory_order_relaxed);
                    if (index >= jobs.size()) {
                        return;
                    }

                    const auto& job = jobs[index];
                    std::cout << "[Info] upload worker " << (worker + 1)
                              << " started: " << job.filename << "\n";
                    if (upload_one_threadsafe(job.local_path, remote_dir, job.filename,
                                              job.file_size, gaopei)) {
                        success.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        failed.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        auto dur = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        double speed = dur > 0 ? (total_bytes / 1024.0 / 1024.0) / dur : 0.0;

        std::cout << "\n 总速度: " << speed << " MB/s\n";
        if (failed.load(std::memory_order_relaxed) > 0) {
            std::cout << "[Error] " << failed.load(std::memory_order_relaxed)
                      << " file(s) failed to upload\n";
        }
        return failed.load(std::memory_order_relaxed) == 0;
    }

    // Upload map files directly in the original order, one request at a time.
    bool upload_files(const std::string& local_dir,
                      const std::vector<std::string>& file_list,
                      const std::string& remote_dir, bool gaopei = false) {
        auto start = std::chrono::steady_clock::now();
        size_t total_bytes = 0;
        size_t uploaded_bytes = 0;

        for (const auto& filename : file_list) {
            fs::path local = fs::path(local_dir) / filename;
            if (!fs::exists(local)) {
                std::cout << "[Warn] skip missing file: " << local.string() << "\n";
                return false;
            }

            const size_t file_size = fs::file_size(local);
            total_bytes += file_size;
            if (!upload_one_threadsafe(local.string(), remote_dir, filename,
                                       file_size, gaopei)) {
                return false;
            }
            uploaded_bytes += file_size;
        }

        auto dur = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        double speed = dur > 0 ? (uploaded_bytes / 1024.0 / 1024.0) / dur : 0.0;
        std::cout << "\n上传完成: " << file_list.size()
                  << " 个文件, " << uploaded_bytes << "/" << total_bytes
                  << " bytes, 速度: " << speed << " MB/s\n";
        return true;
    }

    bool download_files(const std::string& remote_dir,
                        const std::vector<std::string>& file_list,
                        const std::string& local_dir) {
        fs::create_directories(local_dir);

        size_t total_bytes = 0;
        int success = 0;

        auto start = std::chrono::steady_clock::now();

        for (const auto& file : file_list) {
            fs::path local = fs::path(local_dir) / file;
            if (download_one_threadsafe(remote_dir, file, local.string(), total_bytes)) {
                success++;
            }
        }

        auto dur = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        double speed = dur > 0 ? (total_bytes / 1024.0 / 1024.0) / dur : 0.0;

        std::cout << "\n下载完成: " << success << "/" << file_list.size()
                  << " 速度: " << speed << " MB/s\n";

        return success > 0;
    }

private:
    struct CurlGlobalInit {
        CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    };

    struct CurlDeleter {
        void operator()(CURL* curl) const {
            if (curl) {
                curl_easy_cleanup(curl);
            }
        }
    };

    struct CurlSlistDeleter {
        void operator()(curl_slist* list) const {
            if (list) {
                curl_slist_free_all(list);
            }
        }
    };

    struct RequestResult {
        bool ok = false;
        long http_code = 0;
        CURLcode curl_code = CURLE_OK;
        std::string body;
        std::string error_buffer;
    };

    struct UploadContext {
        std::ifstream file;
        size_t file_size = 0;
        std::string filename;
        HttpMapUploader* owner = nullptr;
    };

    struct DownloadContext {
        std::ofstream file;
        size_t received = 0;
        size_t file_size = 0;
        std::string filename;
        HttpMapUploader* owner = nullptr;
        size_t* total_bytes = nullptr;
    };

    static void ensure_curl_global_init() {
        static CurlGlobalInit init_once;
        (void)init_once;
    }

    std::string base_url() const {
        return "http://" + host_ + ":" + std::to_string(port_);
    }

    std::string make_url(const std::string& path) const {
        return base_url() + path;
    }

    void disable_proxy(CURL* curl) const {
        // 直连业务服务，避免继承进程环境中的 http_proxy/https_proxy/all_proxy
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
    }

    RequestResult perform_simple_request(const std::string& method,
                                         const std::string& path,
                                         const std::string* body,
                                         const std::vector<std::string>* headers) {
        RequestResult result;
        std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
        if (!curl) {
            result.error_buffer = "failed to create curl handle";
            return result;
        }

        char error_buffer[CURL_ERROR_SIZE] = {0};
        std::string response_body;
        curl_easy_setopt(curl.get(), CURLOPT_URL, make_url(path).c_str());
        disable_proxy(curl.get());
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_string_callback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);

        if (method == "GET") {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
        } else if (method == "POST") {
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
            if (body) {
                curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body->c_str());
                curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
                                 static_cast<curl_off_t>(body->size()));
            }
        }

        std::unique_ptr<curl_slist, CurlSlistDeleter> list;
        if (headers && !headers->empty()) {
            curl_slist* raw = nullptr;
            for (const auto& header : *headers) {
                raw = curl_slist_append(raw, header.c_str());
            }
            list.reset(raw);
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, list.get());
        }

        result.curl_code = curl_easy_perform(curl.get());
        if (result.curl_code != CURLE_OK) {
            result.error_buffer = error_buffer[0] ? error_buffer : curl_easy_strerror(result.curl_code);
            return result;
        }

        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &result.http_code);
        result.body = std::move(response_body);
        result.ok = true;
        return result;
    }

    static size_t write_string_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* out = static_cast<std::string*>(userdata);
        const size_t bytes = size * nmemb;
        out->append(ptr, bytes);
        return bytes;
    }

    static size_t read_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* ctx = static_cast<UploadContext*>(userdata);
        if (!ctx || !ctx->file) {
            return 0;
        }

        const size_t max_bytes = size * nmemb;
        ctx->file.read(ptr, static_cast<std::streamsize>(max_bytes));
        return static_cast<size_t>(ctx->file.gcount());
    }

    static size_t write_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* ctx = static_cast<DownloadContext*>(userdata);
        if (!ctx || !ctx->file) {
            return 0;
        }

        const size_t bytes = size * nmemb;
        ctx->file.write(ptr, static_cast<std::streamsize>(bytes));
        if (!ctx->file) {
            return 0;
        }

        ctx->received += bytes;
        if (ctx->total_bytes) {
            *ctx->total_bytes += bytes;
        }
        if (ctx->owner) {
            ctx->owner->print_progress(ctx->received, ctx->file_size, ctx->filename);
        }

        return bytes;
    }

    static int xfer_progress_callback(void* clientp,
                                      curl_off_t /*dltotal*/, curl_off_t /*dlnow*/,
                                      curl_off_t ultotal, curl_off_t ulnow) {
        auto* ctx = static_cast<UploadContext*>(clientp);
        if (!ctx || !ctx->owner) {
            return 0;
        }

        size_t total = ctx->file_size;
        if (ultotal > 0) {
            total = static_cast<size_t>(ultotal);
        }

        size_t current = static_cast<size_t>(std::min<curl_off_t>(
            ulnow, static_cast<curl_off_t>(total)));
        ctx->owner->print_progress(current, total, ctx->filename);
        return 0;
    }

    static int download_progress_callback(void* clientp,
                                          curl_off_t dltotal, curl_off_t dlnow,
                                          curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
        auto* ctx = static_cast<DownloadContext*>(clientp);
        if (!ctx || !ctx->owner) {
            return 0;
        }

        size_t total = ctx->file_size;
        if (dltotal > 0) {
            total = static_cast<size_t>(dltotal);
        }

        size_t current = static_cast<size_t>(std::min<curl_off_t>(
            dlnow, static_cast<curl_off_t>(total)));
        ctx->owner->print_progress(current, total, ctx->filename);
        return 0;
    }

    bool upload_one_threadsafe(const std::string& local,
                               const std::string& remote_dir,
                               const std::string& filename,
                               size_t file_size,
                               bool gaopei) {
        constexpr int MAX_RETRY = 5;
        const auto RETRY_DELAY = std::chrono::seconds(2);

        if (!fs::exists(local)) {
            std::cout << "[Error] local file does not exist: " << local << "\n";
            return false;
        }

        auto start = std::chrono::steady_clock::now();
        std::string upload_path = "/upload/" + url_encode_path(remote_dir + "/" + filename);
        if (gaopei) {
            upload_path += "?gaopei=gaopei";
        }
        std::string full_url = make_url(upload_path);

        for (int attempt = 0; attempt < MAX_RETRY; ++attempt) {
            UploadContext ctx;
            ctx.file.open(local, std::ios::binary);
            ctx.file_size = file_size;
            ctx.filename = filename;
            ctx.owner = this;

            if (!ctx.file) {
                std::cout << "[Error] open local failed: " << local
                          << ", errno: " << errno
                          << ", reason: " << std::strerror(errno)
                          << ", exists: " << fs::exists(local) << "\n";
                return false;
            }

            std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
            if (!curl) {
                std::cout << "[Error] failed to create curl handle for upload: " << filename << "\n";
                return false;
            }

            char error_buffer[CURL_ERROR_SIZE] = {0};
            std::string response_body;
            std::unique_ptr<curl_slist, CurlSlistDeleter> headers;
            curl_slist* raw_headers = nullptr;
            raw_headers = curl_slist_append(raw_headers, "Content-Type: application/octet-stream");
            raw_headers = curl_slist_append(raw_headers, "Expect:");
            headers.reset(raw_headers);

            curl_easy_setopt(curl.get(), CURLOPT_URL, full_url.c_str());
            disable_proxy(curl.get());
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, nullptr);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
                             static_cast<curl_off_t>(file_size));
            curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, &HttpMapUploader::read_file_callback);
            curl_easy_setopt(curl.get(), CURLOPT_READDATA, &ctx);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_string_callback);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
            curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
            curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 1800L);
            curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_TCP_NODELAY, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);
            curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &HttpMapUploader::xfer_progress_callback);
            curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &ctx);

            CURLcode code = curl_easy_perform(curl.get());
            long http_code = 0;
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

            if (code == CURLE_OK && http_code >= 200 && http_code < 300) {
                print_progress(file_size, file_size, filename);

                auto dur = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - start).count();
                double speed = dur > 0 ? (file_size / 1024.0 / 1024.0) / dur : 0.0;

                std::cout << "\n[" << filename << "] "
                          << (file_size / 1024.0 / 1024.0) << " MB, "
                          << speed << " MB/s\n";
                return true;
            }

            std::cout << "[Warn] HTTP upload failed: " << filename
                      << " status: " << http_code
                      << ", curl: " << curl_easy_strerror(code)
                      << ", retry " << (attempt + 1) << "/" << MAX_RETRY << "\n";
            if (error_buffer[0] != '\0') {
                std::cout << "[Warn] curl detail: " << error_buffer << "\n";
            }

            if (attempt + 1 < MAX_RETRY) {
                std::this_thread::sleep_for(RETRY_DELAY);
            }
        }

        std::cout << "[Error] upload failed after retries: " << filename << "\n";
        return false;
    }

    std::string url_encode_path(const std::string& path) {
        std::string encoded = url_encode(path);
        std::string slash = "%2F";
        size_t pos = 0;
        while ((pos = encoded.find(slash, pos)) != std::string::npos) {
            encoded.replace(pos, slash.size(), "/");
            ++pos;
        }
        return encoded;
    }

    bool download_one_threadsafe(const std::string& remote_dir,
                                 const std::string& filename,
                                 const std::string& local,
                                 size_t& total_bytes) {
        std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
        if (!curl) {
            std::cout << "[Error] failed to create curl handle for download: " << filename << "\n";
            return false;
        }

        fs::create_directories(fs::path(local).parent_path());

        DownloadContext ctx;
        ctx.file.open(local, std::ios::binary);
        ctx.filename = filename;
        ctx.owner = this;
        ctx.total_bytes = &total_bytes;

        if (!ctx.file) {
            std::cout << "[Error] open local failed: " << local << "\n";
            return false;
        }

        std::string url = "/download_file?remote_dir=" + url_encode(remote_dir)
                        + "&filename=" + url_encode(filename);
        std::string full_url = make_url(url);
        char error_buffer[CURL_ERROR_SIZE] = {0};

        curl_easy_setopt(curl.get(), CURLOPT_URL, full_url.c_str());
        disable_proxy(curl.get());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &HttpMapUploader::write_file_callback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &ctx);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 1800L);
        curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &HttpMapUploader::download_progress_callback);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &ctx);

        auto start = std::chrono::steady_clock::now();
        CURLcode code = curl_easy_perform(curl.get());
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
        curl_off_t content_length = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &content_length);
        ctx.file_size = content_length > 0 ? static_cast<size_t>(content_length) : ctx.received;

        if (code == CURLE_OK && http_code >= 200 && http_code < 300) {
            if (ctx.file_size > 0) {
                print_progress(ctx.received, ctx.file_size, filename);
            }

            auto dur = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            double speed = dur > 0 ? (ctx.received / 1024.0 / 1024.0) / dur : 0.0;

            std::cout << "\n[" << filename << "] "
                      << (ctx.received / 1024.0 / 1024.0) << " MB, "
                      << speed << " MB/s\n";
            return true;
        }

        std::cout << "[Warn] HTTP download failed: " << filename
                  << " status: " << http_code
                  << ", curl: " << curl_easy_strerror(code) << "\n";
        if (error_buffer[0] != '\0') {
            std::cout << "[Warn] curl detail: " << error_buffer << "\n";
        }
        return false;
    }

    void print_progress(size_t current, size_t total, const std::string& filename = "") {
        if (total == 0) {
            return;
        }

        int percent = static_cast<int>((double)current / total * 100);

        double cur_mb = current / 1024.0 / 1024.0;
        double total_mb = total / 1024.0 / 1024.0;
        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(io_mutex_);
        auto& state = progress_states_[filename];
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - state.last_send_time).count();
        bool percent_changed = percent != state.last_percent;
        bool timeout = diff_ms >= 1000;

        if (!percent_changed && !timeout) {
            return;
        }

        state.last_percent = percent;
        state.last_send_time = now;

        std::cout << "\r["
                  << percent << "%] "
                  << std::fixed
                  << std::setprecision(1)
                  << cur_mb << "/"
                  << total_mb
                  << " MB"
                  << std::flush;

        if (progress_callback_ && !filename.empty()) {
            json prog_msg = {
                {"type", "file_progress"},
                {"filename", filename},
                {"current", current},
                {"total", total},
                {"percent", percent}
            };

            progress_callback_(prog_msg.dump());
        }
    }

private:
    std::string host_;
    int port_;
    ProgressCallback progress_callback_;

    struct ProgressState {
        int last_percent = -1;
        std::chrono::steady_clock::time_point last_send_time;
    };

    std::unordered_map<std::string, ProgressState> progress_states_;
    std::mutex io_mutex_;
};
