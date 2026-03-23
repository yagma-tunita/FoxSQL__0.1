#pragma once
#include "fs.h"
#include "Page.h"
#include "BufferPool.h"
#include <fstream>
#include <unordered_map>
#include <mutex>

namespace FoxSQL {

    class StorageManager {
    public:
        StorageManager() : bufferPool_(1024) {
            fs::initDataDir();
        }

        void deleteRecord(const std::string& tableName, size_t rid) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& file = getFileStream(tableName);
            size_t pageId = rid;
            file.seekp(pageId * PAGE_SIZE, std::ios::beg);
            uint32_t len = 0;
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.flush();
        }

        void createTableFile(const std::string& tableName) {
            std::string path = fs::getDataDir() + tableName + ".ft";
            if (fs::fileExists(path)) return;
            std::ofstream file(path, std::ios::binary);
            if (!file) throw SQLException("Failed to create table file");
            writeFileHeader(file);
            file.close();
        }

        void writeRecord(const std::string& tableName, size_t rid, const std::vector<byte>& data) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& file = getFileStream(tableName);
            size_t pageId = rid;
            file.seekp(pageId * PAGE_SIZE, std::ios::beg);
            uint32_t len = static_cast<uint32_t>(data.size());
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(reinterpret_cast<const char*>(data.data()), len);
            file.flush();
        }

        std::vector<byte> readRecord(const std::string& tableName, size_t rid) const {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& file = getFileStream(tableName);
            size_t pageId = rid;
            file.seekg(pageId * PAGE_SIZE, std::ios::beg);
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::vector<byte> data(len);
            file.read(reinterpret_cast<char*>(data.data()), len);
            return data;
        }

        void flushAll() {
            bufferPool_.flushAll();
            for (auto& entry : streams_) {
                entry.second.flush();
            }
        }

    private:
        BufferPool bufferPool_;
        mutable std::unordered_map<std::string, std::fstream> streams_;
        mutable std::mutex mutex_;

        std::fstream& getFileStream(const std::string& tableName) const {
            auto it = streams_.find(tableName);
            if (it == streams_.end()) {
                std::string path = fs::getDataDir() + tableName + ".ft";
                auto& stream = streams_[tableName];
                stream.open(path, std::ios::binary | std::ios::in | std::ios::out);
                if (!stream) throw SQLException("Cannot open table file");
                return stream;
            }
            return it->second;
        }

        void writeFileHeader(std::ofstream& file) {
            uint32_t magic = 0x46534C51; // "FSLQ"
            uint32_t version = 1;
            uint32_t pageSize = PAGE_SIZE;
            file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
            file.write(reinterpret_cast<const char*>(&version), sizeof(version));
            file.write(reinterpret_cast<const char*>(&pageSize), sizeof(pageSize));
        }
    };

}