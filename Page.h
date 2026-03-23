#pragma once
#include "common.h"
#include <cstring>
#include <vector>

namespace FoxSQL {

    constexpr size_t PAGE_SIZE = 4096;

    class Page {
    public:
        Page(size_t id) : pageId_(id), dirty_(false), data_(PAGE_SIZE, 0) {}

        size_t getPageId() const { return pageId_; }
        bool isDirty() const { return dirty_; }
        void markDirty() { dirty_ = true; }
        void clearDirty() { dirty_ = false; }

        void* getData() { return data_.data(); }
        const void* getData() const { return data_.data(); }

        void readFrom(const void* src, size_t offset, size_t len) {
            if (offset + len > PAGE_SIZE) throw SQLException("Page read overflow");
            std::memcpy(data_.data() + offset, src, len);
            dirty_ = true;
        }

        void writeTo(void* dst, size_t offset, size_t len) const {
            if (offset + len > PAGE_SIZE) throw SQLException("Page write overflow");
            std::memcpy(dst, data_.data() + offset, len);
        }

    private:
        size_t pageId_;
        bool dirty_;
        std::vector<byte> data_;
    };

}