#pragma once
#include "Row.h"
#include <vector>

namespace FoxSQL {

    class ResultSet {
    public:
        using Iterator = std::vector<Row>::const_iterator;

        void addRow(Row row) {
            rows_.push_back(std::move(row));
        }

        Iterator begin() const { return rows_.begin(); }
        Iterator end() const { return rows_.end(); }
        size_t size() const { return rows_.size(); }
        bool empty() const { return rows_.empty(); }

        const Row& operator[](size_t i) const { return rows_[i]; }

    private:
        std::vector<Row> rows_;
    };
}