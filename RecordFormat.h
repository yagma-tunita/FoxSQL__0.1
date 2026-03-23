#pragma once
#include "Row.h"
#include <vector>
#include <cstring>

namespace FoxSQL {

    class RecordFormat {
    public:
        static std::vector<byte> serialize(const Row& row, const std::vector<ColumnMeta>& columns) {
            std::vector<byte> data;
            for (const auto& col : columns) {
                const Value& val = row.getValue(col.name);
                if (col.type == ColumnType::INT) {
                    int64_t intVal = val.getInt();
                    const byte* ptr = reinterpret_cast<const byte*>(&intVal);
                    data.insert(data.end(), ptr, ptr + sizeof(int64_t));
                }
                else {
                    const std::string& str = val.getString();
                    uint32_t len = static_cast<uint32_t>(str.size());
                    const byte* lenPtr = reinterpret_cast<const byte*>(&len);
                    data.insert(data.end(), lenPtr, lenPtr + sizeof(uint32_t));
                    data.insert(data.end(), str.begin(), str.end());
                }
            }
            return data;
        }

        static Row deserialize(const byte* data, const std::vector<ColumnMeta>& columns) {
            Row row;
            size_t offset = 0;
            for (const auto& col : columns) {
                if (col.type == ColumnType::INT) {
                    int64_t val;
                    std::memcpy(&val, data + offset, sizeof(int64_t));
                    row.setValue(col.name, Value(val));
                    offset += sizeof(int64_t);
                }
                else {
                    uint32_t len;
                    std::memcpy(&len, data + offset, sizeof(uint32_t));
                    offset += sizeof(uint32_t);
                    std::string str(reinterpret_cast<const char*>(data + offset), len);
                    row.setValue(col.name, Value(str));
                    offset += len;
                }
            }
            return row;
        }

        static size_t getRecordSize(const std::vector<ColumnMeta>& columns, const Row& row) {
            size_t size = 0;
            for (const auto& col : columns) {
                if (col.type == ColumnType::INT) size += sizeof(int64_t);
                else size += sizeof(uint32_t) + row.getValue(col.name).getString().size();
            }
            return size;
        }
    };

} 