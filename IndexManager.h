#pragma once
#include "common.h"
#include "BPlusTree.h"
#include <functional>

namespace FoxSQL
{
    class IndexManager {
    public:
        IndexManager() = default;
        ~IndexManager() { clear(); }

        template <typename K, typename V>
        void createIndex(const std::string& name) {
            auto idx = new BPlusTree<K, V>();
            indices_[name] = idx;
            deleters_[name] = [](void* ptr) { delete static_cast<BPlusTree<K, V>*>(ptr); };
        }

        template <typename K, typename V>
        void insert(const std::string& name, const K& key, const V& value) {
            auto idx = static_cast<BPlusTree<K, V>*>(indices_.at(name));
            idx->insert(key, value);
        }

        template <typename K, typename V>
        V lookup(const std::string& name, const K& key) const {
            auto idx = static_cast<BPlusTree<K, V>*>(indices_.at(name));
            return idx->lookup(key);
        }

        template <typename K, typename V>
        void remove(const std::string& name, const K& key) {
            auto idx = static_cast<BPlusTree<K, V>*>(indices_.at(name));
            idx->remove(key);
        }

        void clear() {
            for (auto& p : deleters_) {
                p.second(indices_[p.first]);
            }
            indices_.clear();
            deleters_.clear();
        }

    private:
        std::unordered_map<std::string, void*> indices_;
        std::unordered_map<std::string, std::function<void(void*)>> deleters_;
    };
}