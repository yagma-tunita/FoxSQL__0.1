#pragma once
#include <string>
#include "fs.h"

namespace FoxSQL {

    class Config {
    public:
        static Config& instance() {
            static Config config;
            return config;
        }

        void setDataDir(const std::string& dir) {
            dataDir_ = dir;
            fs::createDirectories(dataDir_);
        }

        std::string getDataDir() const {
            return dataDir_;
        }

        void setBufferPoolSize(size_t size) {
            bufferPoolSize_ = size;
        }

        size_t getBufferPoolSize() const {
            return bufferPoolSize_;
        }

    private:
        Config() : dataDir_(fs::getDataDir()), bufferPoolSize_(1024) {}
        std::string dataDir_;
        size_t bufferPoolSize_;
    };

}