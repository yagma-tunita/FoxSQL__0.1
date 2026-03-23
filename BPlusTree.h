#pragma once
#include "common.h"
#include "Types.h"
#include <algorithm>
#include <iterator>  
#include <utility>    

namespace FoxSQL
{

    template <typename K, typename V>
    struct BPlusTreeNode {
        bool is_leaf;
        std::vector<K> keys;
        std::vector<V> values;
        std::vector<std::unique_ptr<BPlusTreeNode<K, V>>> children;
        BPlusTreeNode* next;

        explicit BPlusTreeNode(bool leaf) : is_leaf(leaf), next(nullptr) {}
    };

    template <typename K, typename V>
    class BPlusTree {
    public:
        BPlusTree() : root(std::make_unique<BPlusTreeNode<K, V>>(true)) {}

        ~BPlusTree() = default;

        BPlusTree(const BPlusTree&) = delete;
        BPlusTree& operator=(const BPlusTree&) = delete;

        void insert(const K& key, const V& value) {
            if (root->keys.size() == B_PLUS_TREE_MAX_KEYS) {
                auto new_root = std::make_unique<BPlusTreeNode<K, V>>(false);
                new_root->children.push_back(std::move(root));
                root = std::move(new_root);
                splitChild(root.get(), 0);
            }
            insertNonFull(root.get(), key, value);
        }

        V lookup(const K& key) const {
            return search(root.get(), key);
        }

        void remove(const K& key) {
            removeInternal(root.get(), key);
            if (root && !root->is_leaf && root->keys.empty()) {
                root = std::move(root->children[0]);
            }
        }

        void clear() {
            root.reset();
        }

    private:
        std::unique_ptr<BPlusTreeNode<K, V>> root;

        void insertNonFull(BPlusTreeNode<K, V>* node, const K& key, const V& value) {
            int i = static_cast<int>(node->keys.size()) - 1;
            if (node->is_leaf) {
                while (i >= 0 && key < node->keys[i]) --i;
                ++i;
                node->keys.insert(node->keys.begin() + i, key);
                node->values.insert(node->values.begin() + i, value);
                return;
            }
            while (i >= 0 && key < node->keys[i]) --i;
            ++i;
            if (node->children[i]->keys.size() == B_PLUS_TREE_MAX_KEYS) {
                splitChild(node, i);
                if (key > node->keys[i]) ++i;
            }
            insertNonFull(node->children[i].get(), key, value);
        }

        void splitChild(BPlusTreeNode<K, V>* parent, int idx) {
            const size_t mid = B_PLUS_TREE_MAX_KEYS / 2;
            auto child = parent->children[idx].get();
            auto new_node = std::make_unique<BPlusTreeNode<K, V>>(child->is_leaf);

            if (child->is_leaf) 
            {
                new_node->keys.assign(child->keys.begin() + mid, child->keys.end());
                new_node->values.assign(child->values.begin() + mid, child->values.end());
                child->keys.resize(mid);
                child->values.resize(mid);

                parent->keys.insert(parent->keys.begin() + idx, new_node->keys[0]);
                parent->children.insert(parent->children.begin() + idx + 1, std::move(new_node));

                auto target = parent->children[idx + 1].get();
                target->next = child->next;
                child->next = target;
            }
            else 
            {
                new_node->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
                new_node->children.assign(
                    std::make_move_iterator(child->children.begin() + mid + 1),
                    std::make_move_iterator(child->children.end()));
                child->keys.resize(mid);
                child->children.resize(mid + 1);

                parent->keys.insert(parent->keys.begin() + idx, child->keys[mid]);
                parent->children.insert(parent->children.begin() + idx + 1, std::move(new_node));
            }
        }

        V search(BPlusTreeNode<K, V>* node, const K& key) const {
            int i = 0;
            while (i < static_cast<int>(node->keys.size()) && key > node->keys[i]) ++i;
            if (node->is_leaf) {
                if (i < static_cast<int>(node->keys.size()) && node->keys[i] == key)
                    return node->values[i];
                throw SQLException("Key not found");
            }
            if (i < static_cast<int>(node->keys.size()) && node->keys[i] == key)
                ++i;
            return search(node->children[i].get(), key);
        }

        bool removeInternal(BPlusTreeNode<K, V>* node, const K& key) {
            int i = 0;
            while (i < static_cast<int>(node->keys.size()) && key > node->keys[i]) ++i;

            if (node->is_leaf) {
                if (i < static_cast<int>(node->keys.size()) && node->keys[i] == key) {
                    node->keys.erase(node->keys.begin() + i);
                    node->values.erase(node->values.begin() + i);
                    return node->keys.size() < B_PLUS_TREE_MIN_KEYS;
                }
                throw SQLException("Key not found");
            }

            bool need_fix = false;
            bool exact_match = (i < static_cast<int>(node->keys.size()) && node->keys[i] == key);
            if (exact_match) {
                auto* right_child = node->children[i + 1].get();
                K successor = findMinKey(right_child);
                node->keys[i] = successor;
                need_fix = removeInternal(right_child, successor);
                if (need_fix) fixChild(node, i + 1);
            }
            else {
                need_fix = removeInternal(node->children[i].get(), key);
                if (need_fix) fixChild(node, i);
            }
            return node != root.get() && node->keys.size() < B_PLUS_TREE_MIN_KEYS;
        }

        void fixChild(BPlusTreeNode<K, V>* parent, int idx) {
            auto child = parent->children[idx].get();
            if (idx > 0 && parent->children[idx - 1]->keys.size() > B_PLUS_TREE_MIN_KEYS) {
                borrowFromLeft(parent, idx);
                return;
            }
            if (idx + 1 < static_cast<int>(parent->children.size()) &&
                parent->children[idx + 1]->keys.size() > B_PLUS_TREE_MIN_KEYS) {
                borrowFromRight(parent, idx);
                return;
            }
            if (idx > 0)
                mergeNodes(parent, idx - 1);
            else
                mergeNodes(parent, idx);
        }

        void borrowFromLeft(BPlusTreeNode<K, V>* parent, int idx) {
            auto left = parent->children[idx - 1].get();
            auto child = parent->children[idx].get();
            child->keys.insert(child->keys.begin(), parent->keys[idx - 1]);
            if (child->is_leaf) {
                child->values.insert(child->values.begin(), left->values.back());
                left->values.pop_back();
            }
            else {
                child->children.insert(child->children.begin(), std::move(left->children.back()));
                left->children.pop_back();
            }
            parent->keys[idx - 1] = left->keys.back();
            left->keys.pop_back();
        }

        void borrowFromRight(BPlusTreeNode<K, V>* parent, int idx) {
            auto right = parent->children[idx + 1].get();
            auto child = parent->children[idx].get();
            child->keys.push_back(parent->keys[idx]);
            if (child->is_leaf) {
                child->values.push_back(right->values.front());
                right->values.erase(right->values.begin());
            }
            else {
                child->children.push_back(std::move(right->children.front()));
                right->children.erase(right->children.begin());
            }
            parent->keys[idx] = right->keys.front();
            right->keys.erase(right->keys.begin());
        }

        void mergeNodes(BPlusTreeNode<K, V>* parent, int left_idx) {
            auto left = parent->children[left_idx].get();
            auto right = parent->children[left_idx + 1].get();

            if (left->is_leaf) {
                // 叶子节点：直接合并 keys 和 values
                left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
                left->values.insert(left->values.end(), right->values.begin(), right->values.end());
                left->next = right->next;
            }
            else {
                // 内部节点：先将父节点 key 下移，再合并右节点的 keys 和 children
                left->keys.push_back(parent->keys[left_idx]);
                left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
                left->children.insert(left->children.end(),
                    std::make_move_iterator(right->children.begin()),
                    std::make_move_iterator(right->children.end()));
            }

            // 删除父节点中的 key 和右子节点
            parent->keys.erase(parent->keys.begin() + left_idx);
            parent->children.erase(parent->children.begin() + left_idx + 1);
        }

        K findMinKey(BPlusTreeNode<K, V>* node) const {
            while (!node->is_leaf) node = node->children[0].get();
            return node->keys[0];
        }
    };

}