#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <memory>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <iostream>
#include <utility>

namespace fs = std::filesystem;

class LogManager {
public:
    static LogManager& instance() {
        static LogManager instance;
        return instance;
    }

    std::shared_ptr<spdlog::logger> get_logger() {
        return logger_;
    }

    std::string get_log_file_path() const {
        return log_file_path_;
    }

private:
    LogManager() {
        init_log_dir();
        cleanup_old_logs();
        init_logger();
    }

    ~LogManager() {
        spdlog::shutdown();
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    const std::string log_dir_ = "/home/jianmi/jm/log";
    const size_t max_files_ = 30;
    std::string log_file_path_;

private:
    void init_log_dir() {
        try {
            if (!fs::exists(log_dir_)) {
                fs::create_directories(log_dir_);
            }
        } catch (const std::exception& e) {
            std::cerr << "创建日志目录失败: " << e.what() << std::endl;
        }
    }

    std::string get_log_filename() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_r(&t, &tm);

        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "robot_%Y_%m_%d.log", &tm);

        return log_dir_ + "/" + buffer;
    }

    void init_logger() {
        try {
            std::string filename = get_log_filename();
            log_file_path_ = filename;

            // 使用 basic_file_sink（你是按天生成文件名，不需要 daily sink）
            logger_ = spdlog::basic_logger_mt("robot_logger", filename);

            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%n] %v");

            logger_->set_level(spdlog::level::info);
            logger_->flush_on(spdlog::level::info);

            spdlog::set_default_logger(logger_);

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "日志初始化失败: " << ex.what() << std::endl;
        }
    }

    void cleanup_old_logs() {
        try {
            std::vector<fs::directory_entry> log_files;

            for (auto& p : fs::directory_iterator(log_dir_)) {
                if (p.is_regular_file() && p.path().extension() == ".log") {
                    log_files.push_back(p);
                }
            }

            // 按最后修改时间排序（旧 → 新）
            std::sort(log_files.begin(), log_files.end(),
                      [](const fs::directory_entry& a, const fs::directory_entry& b) {
                          return fs::last_write_time(a) < fs::last_write_time(b);
                      });

            // 删除多余的旧文件
            if (log_files.size() > max_files_) {
                size_t remove_count = log_files.size() - max_files_;
                for (size_t i = 0; i < remove_count; ++i) {
                    fs::remove(log_files[i]);
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "清理日志失败: " << e.what() << std::endl;
        }
    }
};

namespace robot_log {
template <typename... Args>
inline void info(Args&&... args) {
    auto logger = LogManager::instance().get_logger();
    if (logger) {
        logger->info(std::forward<Args>(args)...);
    }
}

template <typename... Args>
inline void warn(Args&&... args) {
    auto logger = LogManager::instance().get_logger();
    if (logger) {
        logger->warn(std::forward<Args>(args)...);
    }
}

template <typename... Args>
inline void error(Args&&... args) {
    auto logger = LogManager::instance().get_logger();
    if (logger) {
        logger->error(std::forward<Args>(args)...);
    }
}

template <typename... Args>
inline void debug(Args&&... args) {
    auto logger = LogManager::instance().get_logger();
    if (logger) {
        logger->debug(std::forward<Args>(args)...);
    }
}
}

#define LOG_INFO(...)  robot_log::info(__VA_ARGS__)
#define LOG_WARN(...)  robot_log::warn(__VA_ARGS__)
#define LOG_ERROR(...) robot_log::error(__VA_ARGS__)
#define LOG_DEBUG(...) robot_log::debug(__VA_ARGS__)
