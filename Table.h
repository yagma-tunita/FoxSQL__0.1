#pragma once
#include "common.h"
#include "Row.h"
#include "RecordFormat.h"
#include "StorageManager.h"
#include "IndexManager.h"
#include <vector>
#include <mutex>
#include <algorithm>
#include <fstream>

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
            storage_.createTableFile(name_);
            index_manager_.createIndex<int64_t, size_t>("primary");
            loadMetadata();
        }

        size_t insertRow(const Row& row) 
        {
            int64_t key = getPrimaryKeyValue(row);
            std::lock_guard<std::mutex> lock(mutex_);

            try 
            {
                index_manager_.lookup<int64_t, size_t>("primary", key);
                throw SQLException("Duplicate primary key");
            }
            catch (const SQLException&) {}

            std::vector<byte> data = RecordFormat::serialize(row, columns_);
            size_t rid = allocateRID();
            storage_.writeRecord(name_, rid, data);

            index_manager_.insert("primary", key, rid);
            return rid;
        }

        Row getRowByPrimaryKey(int64_t key) const {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t rid = index_manager_.lookup<int64_t, size_t>("primary", key);
            return readRecord(rid);
        }

        void deleteRowByPrimaryKey(int64_t key) {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t rid = index_manager_.lookup<int64_t, size_t>("primary", key);
            storage_.deleteRecord(name_, rid);
            index_manager_.remove<int64_t, size_t>("primary", key);
            auto it = std::find(rids_.begin(), rids_.end(), rid);
            if (it != rids_.end()) rids_.erase(it);
        }

        void updateRowByPrimaryKey(int64_t key, const Row& newRow) {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t rid = index_manager_.lookup<int64_t, size_t>("primary", key);
            std::vector<byte> data = RecordFormat::serialize(newRow, columns_);
            storage_.writeRecord(name_, rid, data);
        }

        const std::vector<size_t>& getAllRids() const {
            return rids_;
        }

        const std::vector<ColumnMeta>& getColumns() const { return columns_; }
        const std::string& getName() const { return name_; }

    private:
        std::string name_;
        std::vector<ColumnMeta> columns_;
        StorageManager& storage_;
        IndexManager index_manager_;
        mutable std::mutex mutex_;
        int primary_col_idx_ = -1;
        size_t next_rid_ = 1;
        std::vector<size_t> rids_;   

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

        void loadMetadata() {
            std::string path = fs::getDataDir() + name_ + ".ft";
            std::ifstream file(path, std::ios::binary);
            if (!file) return;

            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            file.seekg(12); 
            size_t pageId = 1;
            size_t offset = PAGE_SIZE;
            rids_.clear();
            while (offset < fileSize) {
                file.seekg(offset, std::ios::beg);
                uint32_t len;
                file.read(reinterpret_cast<char*>(&len), sizeof(len));
                if (file && len > 0 && len < PAGE_SIZE - sizeof(len)) {
                    rids_.push_back(pageId);
                }
                ++pageId;
                offset += PAGE_SIZE;
            }

            for (size_t rid : rids_) {
                try {
                    auto data = storage_.readRecord(name_, rid);
                    Row row = RecordFormat::deserialize(data.data(), columns_);
                    int64_t key = getPrimaryKeyValue(row);
                    index_manager_.insert("primary", key, rid);
                }
                catch (const std::exception&) {}
            }

            if (!rids_.empty()) {
                next_rid_ = *std::max_element(rids_.begin(), rids_.end()) + 1;
            }
            else {
                next_rid_ = 1;
            }
        }

        size_t allocateRID() {
            size_t rid = next_rid_++;
            rids_.push_back(rid);
            return rid;
        }

        Row readRecord(size_t rid) const {
            auto data = storage_.readRecord(name_, rid);
            return RecordFormat::deserialize(data.data(), columns_);
        }
    };

} 