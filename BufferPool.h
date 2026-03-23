#pragma once
#include "Page.h"
#include <unordered_map>
#include <list>
#include <mutex>

namespace FoxSQL {

    class BufferPool {
    public:
        BufferPool(size_t capacity) : capacity_(capacity) {}

        Page* fetchPage(size_t pageId) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pages_.find(pageId);
            if (it != pages_.end()) {
                lruList_.splice(lruList_.begin(), lruList_, it->second.lruIter);
                return it->second.page.get();
            }
            auto page = std::make_unique<Page>(pageId);
            Page* raw = page.get();
            pages_[pageId] = { std::move(page), lruList_.begin() };
            lruList_.push_front(pageId);
            if (pages_.size() > capacity_) evict();
            return raw;
        }

        void markDirty(Page* page) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pages_.find(page->getPageId());
            if (it != pages_.end()) it->second.page->markDirty();
        }

        void flushAll() {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& entry : pages_) {
                if (entry.second.page->isDirty())
                {
                    entry.second.page->clearDirty();
                }
            }
        }

    private:
        struct PageEntry {
            std::unique_ptr<Page> page;
            std::list<size_t>::iterator lruIter;
        };
        size_t capacity_;
        std::unordered_map<size_t, PageEntry> pages_;
        std::list<size_t> lruList_;
        std::mutex mutex_;

        void evict() {
            size_t victim = lruList_.back();
            lruList_.pop_back();
            auto it = pages_.find(victim);
            if (it != pages_.end() && it->second.page->isDirty()) {
            }
            pages_.erase(it);
        }
    };

} // namespace FoxSQL