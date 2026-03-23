#pragma once
#include "common.h"
#include <unordered_map>
#include <string>

namespace FoxSQL
{

    struct Value
    {
        enum class Type { INT, VARCHAR } type;

        union Data {
            int64_t int_val;
            std::string str_val;

            Data() {} 
            ~Data() {} 
        } data;

        Value() : type(Type::INT) {
            new (&data.int_val) int64_t(0);
        }

        Value(int64_t val) : type(Type::INT) {
            new (&data.int_val) int64_t(val);
        }

        Value(std::string val) : type(Type::VARCHAR) {
            new (&data.str_val) std::string(std::move(val));
        }

        Value(const Value& other) : type(other.type) {
            if (type == Type::INT)
                data.int_val = other.data.int_val;
            else
                new (&data.str_val) std::string(other.data.str_val);
        }

        Value(Value&& other) noexcept : type(other.type) {
            if (type == Type::INT)
                data.int_val = other.data.int_val;
            else
                new (&data.str_val) std::string(std::move(other.data.str_val));
        }

        ~Value() {
            if (type == Type::VARCHAR)
                data.str_val.~basic_string();
        }

        Value& operator=(const Value& other) {
            if (this != &other) {
                this->~Value();
                new (this) Value(other);
            }
            return *this;
        }

        Value& operator=(Value&& other) noexcept {
            if (this != &other) {
                this->~Value();
                new (this) Value(std::move(other));
            }
            return *this;
        }

        int64_t getInt() const {
            if (type != Type::INT) throw SQLException("Value is not an INT");
            return data.int_val;
        }

        const std::string& getString() const {
            if (type != Type::VARCHAR) throw SQLException("Value is not a VARCHAR");
            return data.str_val;
        }
    };

    class Row {
    public:
        void setValue(const std::string& col, Value val) {
            values_[col] = std::move(val);
        }

        const Value& getValue(const std::string& col) const {
            auto it = values_.find(col);
            if (it == values_.end()) throw SQLException("Column not found");
            return it->second;
        }

        bool hasColumn(const std::string& col) const {
            return values_.count(col) > 0;
        }

    private:
        std::unordered_map<std::string, Value> values_;
    };

}