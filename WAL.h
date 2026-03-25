#pragma once
#include "common.h"
#include "Row.h"
#include "RecordFormat.h"
#include "StorageManager.h"
#include "IndexManager.h"
#include <unordered_map>
#include <mutex>
#include <algorithm>

namespace FoxSQL
{
    class Table
    {
    public:
        Table(const std::string& name,
            const std::vector<ColumnMeta>& columns,
            StorageManager& storage)
            : name_(name), columns_(columns), storage_(storage) {
            validatePrimaryKey();
            loadFromDisk();               // 从磁盘加载所有行到内存
            // 重建索引（基于内存数据）
            index_manager_.createIndex<int64_t, size_t>("primary");
            rebuildIndex();
        }

        // 插入行（仅修改内存）
        size_t insertRow(const Row& row)
        {
            int64_t key = getPrimaryKeyValue(row);
            std::lock_guard<std::mutex> lock(mutex_);

            if (rows_.find(key) != rows_.end())
                throw SQLException("Duplicate primary key");

            size_t rid = next_rid_++;
            rows_[key] = row;
            index_manager_.insert("primary", key, rid);
            return rid;
        }

        // 通过主键获取行（从内存）
        Row getRowByPrimaryKey(int64_t key) const {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = rows_.find(key);
            if (it == rows_.end())
                throw SQLException("Row not found");
            return it->second;
        }

        // 删除行（仅修改内存）
        void deleteRowByPrimaryKey(int64_t key) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = rows_.find(key);
            if (it == rows_.end())
                throw SQLException("Row not found");
            rows_.erase(it);
            index_manager_.remove<int64_t, size_t>("primary", key);
        }

        // 更新行（仅修改内存）
        void updateRowByPrimaryKey(int64_t key, const Row& newRow) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = rows_.find(key);
            if (it == rows_.end())
                throw SQLException("Row not found");
            it->second = newRow;
        }

        // 获取所有主键（用于全表扫描，模拟 RID 列表）
        std::vector<int64_t> getAllKeys() const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<int64_t> keys;
            keys.reserve(rows_.size());
            for (const auto& p : rows_)
                keys.push_back(p.first);
            return keys;
        }

        const std::vector<ColumnMeta>& getColumns() const { return columns_; }
        const std::string& getName() const { return name_; }

        // 保存所有内存数据到磁盘（由 Database 在 exit 时调用）
        void persistToDisk() {
            std::lock_guard<std::mutex> lock(mutex_);
            // 先清空文件（重建）
            storage_.createTableFile(name_, true);   // truncate = true
            size_t rid = 1;
            for (const auto& p : rows_) {
                const Row& row = p.second;
                std::vector<byte> data = RecordFormat::serialize(row, columns_);
                storage_.writeRecord(name_, rid++, data);
            }
            storage_.flushAll();
        }

    private:
        std::string name_;
        std::vector<ColumnMeta> columns_;
        StorageManager& storage_;
        IndexManager index_manager_;
        mutable std::mutex mutex_;
        int primary_col_idx_ = -1;
        size_t next_rid_ = 1;
        std::unordered_map<int64_t, Row> rows_;   // 主键 -> 行

        int64_t getPrimaryKeyValue(const Row& row) const {
            const Value& val = row.getValue(columns_[primary_col_idx_].name);
            return val.getInt();
        }

        void validatePrimaryKey() {
            for (size_t i = 0; i < columns_.size(); ++i) {
                if (columns_[i].is_primary) {
                    if (primary_col_idx_ != -1)
                        throw SQLException("Multiple primary keys");
                    primary_col_idx_ = static_cast<int>(i);
                }
            }
            if (primary_col_idx_ == -1)
                throw SQLException("No primary key");
        }

        void loadFromDisk() {
            std::string path = fs::getDataDir() + name_ + ".ft";
            std::ifstream file(path, std::ios::binary);
            if (!file) return;   // 新表，无数据

            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            file.seekg(12);       // 跳过文件头 (magic+version+pageSize)

            size_t pageId = 1;
            size_t offset = PAGE_SIZE;
            while (offset < fileSize) {
                file.seekg(offset, std::ios::beg);
                uint32_t len;
                file.read(reinterpret_cast<char*>(&len), sizeof(len));
                if (file && len > 0 && len < PAGE_SIZE - sizeof(len)) {
                    std::vector<byte> data(len);
                    file.read(reinterpret_cast<char*>(data.data()), len);
                    Row row = RecordFormat::deserialize(data.data(), columns_);
                    int64_t key = getPrimaryKeyValue(row);
                    rows_[key] = row;
                    if (pageId >= next_rid_) next_rid_ = pageId + 1;
                }
                ++pageId;
                offset += PAGE_SIZE;
            }
        }

        void rebuildIndex() {
            index_manager_.createIndex<int64_t, size_t>("primary");
            size_t rid = 1;
            for (const auto& p : rows_) {
                index_manager_.insert("primary", p.first, rid++);
            }
            if (rid > next_rid_) next_rid_ = rid;
        }
    };
}