// Catalog.h
#pragma once
#include "common.h"
#include "StorageManager.h"
#include "RecordFormat.h"
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <cstring>

namespace FoxSQL {

    class Catalog {
    public:
        static Catalog& instance() {
            static Catalog cat;
            return cat;
        }

        void init(StorageManager& storage) {
            storage_ = &storage;
            loadFromFile();
        }

        void createTable(const std::string& name, const std::vector<ColumnMeta>& columns) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (tables_.count(name)) throw SQLException("Table already exists");
            tables_[name] = columns;
            persistAllTables();
        }

        const std::vector<ColumnMeta>& getTableColumns(const std::string& name) const {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tables_.find(name);
            if (it == tables_.end()) throw SQLException("Table not found");
            return it->second;
        }

        bool tableExists(const std::string& name) const {
            std::lock_guard<std::mutex> lock(mutex_);
            return tables_.count(name) > 0;
        }

        std::vector<std::string> getAllTableNames() const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<std::string> names;
            for (const auto& p : tables_) names.push_back(p.first);
            return names;
        }

    private:
        std::unordered_map<std::string, std::vector<ColumnMeta>> tables_;
        StorageManager* storage_ = nullptr;
        mutable std::mutex mutex_;

        Catalog() = default;

        void loadFromFile() {
            std::string path = fs::getDataDir() + "catalog.ft";
            std::ifstream file(path, std::ios::binary);
            if (!file) return;

            uint32_t magic;
            file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            if (magic != 0x4341544C) return; // "CATL"
            uint32_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (version != 1) return;

            uint32_t tableCount;
            file.read(reinterpret_cast<char*>(&tableCount), sizeof(tableCount));

            for (uint32_t t = 0; t < tableCount; ++t) {
                uint32_t nameLen;
                file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
                std::string name(nameLen, ' ');
                file.read(&name[0], nameLen);
                uint32_t colCount;
                file.read(reinterpret_cast<char*>(&colCount), sizeof(colCount));
                std::vector<ColumnMeta> columns;
                for (uint32_t c = 0; c < colCount; ++c) {
                    uint32_t colNameLen;
                    file.read(reinterpret_cast<char*>(&colNameLen), sizeof(colNameLen));
                    std::string colName(colNameLen, ' ');
                    file.read(&colName[0], colNameLen);
                    uint32_t typeInt;
                    file.read(reinterpret_cast<char*>(&typeInt), sizeof(typeInt));
                    ColumnType type = static_cast<ColumnType>(typeInt);
                    uint64_t maxLen;
                    file.read(reinterpret_cast<char*>(&maxLen), sizeof(maxLen));
                    uint8_t isPrimaryByte;
                    file.read(reinterpret_cast<char*>(&isPrimaryByte), sizeof(isPrimaryByte));
                    bool isPrimary = isPrimaryByte != 0;
                    columns.emplace_back(colName, type, maxLen, isPrimary);
                }
                tables_[name] = std::move(columns);
            }
        }

        void persistAllTables() {
            std::string path = fs::getDataDir() + "catalog.ft";
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) throw SQLException("Failed to open catalog file for writing");

            uint32_t magic = 0x4341544C; // "CATL"
            uint32_t version = 1;
            file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
            file.write(reinterpret_cast<const char*>(&version), sizeof(version));

            uint32_t tableCount = static_cast<uint32_t>(tables_.size());
            file.write(reinterpret_cast<const char*>(&tableCount), sizeof(tableCount));

            for (const auto& entry : tables_) {
                const std::string& tname = entry.first;
                const std::vector<ColumnMeta>& cols = entry.second;
                uint32_t nameLen = static_cast<uint32_t>(tname.size());
                file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
                file.write(tname.c_str(), nameLen);
                uint32_t colCount = static_cast<uint32_t>(cols.size());
                file.write(reinterpret_cast<const char*>(&colCount), sizeof(colCount));
                for (const auto& col : cols) {
                    uint32_t colNameLen = static_cast<uint32_t>(col.name.size());
                    file.write(reinterpret_cast<const char*>(&colNameLen), sizeof(colNameLen));
                    file.write(col.name.c_str(), colNameLen);
                    uint32_t typeInt = static_cast<uint32_t>(col.type);
                    file.write(reinterpret_cast<const char*>(&typeInt), sizeof(typeInt));
                    uint64_t maxLen = static_cast<uint64_t>(col.max_length);
                    file.write(reinterpret_cast<const char*>(&maxLen), sizeof(maxLen));
                    uint8_t isPrimaryByte = col.is_primary ? 1 : 0;
                    file.write(reinterpret_cast<const char*>(&isPrimaryByte), sizeof(isPrimaryByte));
                }
            }
            file.flush();
        }
    };

} 