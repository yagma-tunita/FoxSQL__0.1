#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>
#include "fs.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) 
#endif

namespace FoxSQL {

    class Logger {
    public:
        enum Level { INFO, WARN, ERR };

        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        void init(const std::string& filename = "foxsql.log") {
            std::string fullPath = fs::getDataDir() + filename;
            setLogFile(fullPath);
        }

        void setLogFile(const std::string& filepath) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (logFile_.is_open()) logFile_.close();
            logFile_.open(filepath, std::ios::app);
            if (!logFile_) {
                fs::createDirectories(fs::getDataDir());
                logFile_.open(filepath, std::ios::app);
            }
        }

        void log(Level level, const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string timeStr = std::ctime(&time_t);
            timeStr.pop_back();

            std::string prefix;
            switch (level) {
            case INFO: prefix = "[INFO] "; break;
            case WARN: prefix = "[WARN] "; break;
            case ERR:  prefix = "[ERROR] "; break;
            }

            std::string line = timeStr + " " + prefix + msg + "\n";
            if (logFile_.is_open()) logFile_ << line;
            std::cout << line;
        }

    private:
        Logger() = default;
        ~Logger() { if (logFile_.is_open()) logFile_.close(); }

        std::ofstream logFile_;
        std::mutex mutex_;
    };

#define LOG_INFO(msg) FoxSQL::Logger::instance().log(FoxSQL::Logger::INFO, msg)
#define LOG_WARN(msg) FoxSQL::Logger::instance().log(FoxSQL::Logger::WARN, msg)
#define LOG_ERR(msg)  FoxSQL::Logger::instance().log(FoxSQL::Logger::ERR, msg)

} 