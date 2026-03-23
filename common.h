#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <cstdint>

namespace FoxSQL
{

    enum class ColumnType 
    {
        INT,
        VARCHAR
    };

    struct ColumnMeta 
    {
        std::string name;
        ColumnType type;
        size_t max_length;
        bool is_primary;

        ColumnMeta(std::string n, ColumnType t, size_t len, bool primary)
            : name(std::move(n)), type(t), max_length(len), is_primary(primary){}
    };

    class SQLException : public std::runtime_error 
    {
    public:
        explicit SQLException(const std::string& msg) : std::runtime_error(msg) {}
    };

    using byte = unsigned char;

}