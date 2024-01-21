#include <algorithm>
#include <stdexcept>
#include "BTree.h"

template<typename T>
void erase_by_value(std::vector<T> &vector, const T &value) {
    auto iter = std::find(vector.begin(), vector.end(), value);
    if (iter != vector.end()) {
        vector.erase(iter);
    }
}

tree::BTree::Node::Node(int factor) : factor(factor),
                                      keys(0),
                                      children(0),
                                      parent(nullptr),
                                      is_leaf(false) {}

bool tree::BTree::Node::is_full() const {
    return keys.size() == 2 * factor - 1;
}

tree::BTree::BTree(int factor) : factor_(factor), root_(nullptr) {}

void tree::BTree::insert(int key) {
    if (root_ == nullptr) {
        root_ = std::make_shared<Node>(factor_);
        root_->keys.push_back(key);
        root_->is_leaf = true;
        return;
    }

    auto current_node = root_;

    while (!current_node->is_leaf) {
        auto index = static_cast<int>(std::lower_bound(current_node->keys.begin(),
                                                       current_node->keys.end(),
                                                       key) - current_node->keys.begin());
        auto next_node = current_node->children[index];

        if (current_node->is_full()) {
            split(current_node);
        }

        current_node = next_node;
    }

    // дошли до листа
    if (current_node->is_full()) {
        split(current_node);

        auto dad = current_node->parent;
        auto index_in_dad = static_cast<int>(std::lower_bound(dad->keys.begin(),
                                                              dad->keys.end(),
                                                              key) - dad->keys.begin());
        auto node_for_insert = dad->children[index_in_dad];

        node_for_insert->keys.insert(std::lower_bound(node_for_insert->keys.begin(),
                                                      node_for_insert->keys.end(),
                                                      key),
                                     key);

    } else {
        current_node->keys.insert(std::lower_bound(current_node->keys.begin(),
                                                   current_node->keys.end(),
                                                   key),
                                  key);
    }


}

void tree::BTree::split(const tree::BTree::NodePtr &node) {  // node станет левой нодой
    if (node->parent == nullptr) {  // если сплит корня
        auto r_middle_key = node->keys[factor_ - 1];
        auto r_right_node = std::make_shared<Node>(factor_);

        for (auto i = factor_; i < 2 * factor_ - 1; ++i) {
            r_right_node->keys.push_back(node->keys[i]);
        }
        node->keys.resize(factor_ - 1);

        if (!node->is_leaf) {
            for (auto i = factor_; i < 2 * factor_; ++i) {
                r_right_node->children.push_back(node->children[i]);
                node->children[i]->parent = r_right_node;
            }
            node->children.resize(factor_);
        }

        auto new_root = std::make_shared<Node>(factor_);
        new_root->keys.push_back(r_middle_key);
        new_root->children.push_back(node);
        new_root->children.push_back(r_right_node);

        node->parent = new_root;
        r_right_node->parent = new_root;

        r_right_node->is_leaf = node->is_leaf;

        root_ = new_root;
        return;
    }

    auto middle_key = node->keys[factor_ - 1];
    auto right_node = std::make_shared<Node>(factor_);

    // переносим ключи
    for (auto i = factor_; i < 2 * factor_ - 1; ++i) {
        right_node->keys.push_back(node->keys[i]);
    }
    node->keys.resize(factor_ - 1);

    // переносим детей
    if (!node->is_leaf) {
        for (auto i = factor_; i < 2 * factor_; ++i) {
            right_node->children.push_back(node->children[i]);
            node->children[i]->parent = right_node;
        }
        node->children.resize(factor_);
    }

    right_node->parent = node->parent;
    right_node->is_leaf = node->is_leaf;

    // найдем в какое место воткнуть мидл
    auto index = static_cast<int>(std::lower_bound(node->parent->keys.begin(),
                                                   node->parent->keys.end(),
                                                   middle_key) - node->parent->keys.begin());
    node->parent->keys.insert(node->parent->keys.begin() + index, middle_key);
    node->parent->children.insert(node->parent->children.begin() + index + 1, right_node);
}

tree::BTree::NodePtr tree::BTree::merge(const tree::BTree::NodePtr &left,
                                        const tree::BTree::NodePtr &right,
                                        int middle_key) {
    auto result = std::make_shared<Node>(factor_);

    // переносим ключи
    for (auto key: left->keys) {
        result->keys.push_back(key);
    }
    result->keys.push_back(middle_key);
    for (auto key: right->keys) {
        result->keys.push_back(key);
    }

    result->is_leaf = left->is_leaf;  // используется только когда left и right на одной высоте

    if (!result->is_leaf) {
        // переносим детей
        for (const auto &child: left->children) {
            result->children.push_back(child);
            child->parent = result;
        }
        for (const auto &child: right->children) {
            result->children.push_back(child);
            child->parent = result;
        }
    }

    result->parent = left->parent;

    return result;
}

bool tree::BTree::move_key(const tree::BTree::NodePtr &target_node) {
    if (target_node == root_) {
        throw std::runtime_error("Хз че делать подумаю потом по идее такое вообще не возникнет");
    }

    auto dad = target_node->parent;
    auto index_in_dad = static_cast<int>(std::lower_bound(dad->keys.begin(),
                                                          dad->keys.end(),
                                                          target_node->keys.front()) - dad->keys.begin());

    if (index_in_dad > 0 && dad->children[index_in_dad - 1]->keys.size() >= factor_) {
        auto left_brother = dad->children[index_in_dad - 1];
        auto lb_max_key = left_brother->keys.back();
        auto lb_max_child = left_brother->children.back();

        dad->keys.insert(dad->keys.begin() + index_in_dad - 1, lb_max_key);
        target_node->keys.insert(target_node->keys.begin(), dad->keys[index_in_dad]);
        target_node->children.insert(target_node->children.begin(), lb_max_child);
        lb_max_child->parent = target_node;

        dad->keys.erase(dad->keys.begin() + index_in_dad);
        left_brother->keys.erase(left_brother->keys.end() - 1);
        left_brother->children.erase(left_brother->children.end() - 1);
    } else if (index_in_dad < dad->children.size() - 1 && dad->children[index_in_dad + 1]->keys.size() >= factor_) {
        auto right_brother = dad->children[index_in_dad + 1];
        auto rb_min_key = right_brother->keys.front();
        auto rb_min_child = right_brother->children.front();

        dad->keys.insert(dad->keys.begin() + index_in_dad + 1, rb_min_key);
        target_node->keys.push_back(dad->keys[index_in_dad]);
        target_node->children.push_back(rb_min_child);
        rb_min_child->parent = target_node;

        dad->keys.erase(dad->keys.begin() + index_in_dad);
        right_brother->keys.erase(right_brother->keys.begin());
        right_brother->children.erase(right_brother->children.begin());
    } else {
        return false;  // move не произошел
    }

    return true;
}

void tree::BTree::remove(int key) {
    if (root_ == nullptr) return;

    if (root_->is_leaf) {
        erase_by_value(root_->keys, key);
        return;
    }

    auto current_node = root_;

    while (true) {
        if (std::find(current_node->keys.begin(),
                      current_node->keys.end(),
                      key) != current_node->keys.end()) {
            break;
        }

        if (current_node->is_leaf) return;

        auto index = static_cast<int>(std::lower_bound(current_node->keys.begin(),
                                                       current_node->keys.end(),
                                                       key) - current_node->keys.begin());

        auto next_node = current_node->children[index];

        if (current_node != root_ && current_node->keys.size() < factor_) {
            auto move_result = move_key(current_node);

            if (!move_result) {
                auto dad = current_node->parent;
                auto index_in_dad = static_cast<int>(std::lower_bound(dad->keys.begin(),
                                                                      dad->keys.end(),
                                                                      current_node->keys.front())
                                                     - dad->keys.begin());
                NodePtr new_node;
                int index_for_new_node;
                if (index_in_dad > 0) {
                    new_node = merge(dad->children[index_in_dad - 1],
                                     current_node, dad->keys[index_in_dad - 1]);
                    index_for_new_node = index_in_dad - 1;
                } else if (index_in_dad < dad->children.size() - 1) {
                    new_node = merge(current_node,
                                     dad->children[index_in_dad + 1],
                                     dad->keys[index_in_dad]);
                    index_for_new_node = index_in_dad;
                } else {
                    throw std::runtime_error("Ну такого не бывает либо левый либо правый брат должен быть");
                }

                dad->keys.erase(dad->keys.begin() + index_for_new_node);
                dad->children[index_for_new_node] = new_node;
                dad->children.erase(dad->children.begin() + index_for_new_node + 1);
            }
        }

        current_node = next_node;
    }

    if (current_node->is_leaf) {
        remove_from_leaf(current_node, key);
    } else {
        remove_from_inner(current_node, key);
    }
}

void tree::BTree::remove_from_leaf(const tree::BTree::NodePtr &node, int key) {
    if (node->keys.size() > factor_ - 1) {
        erase_by_value(node->keys, key);
        return;
    }

    auto dad = node->parent;  // он всегда есть потому что случай "корень - лист" разбирается в remove
    auto index_in_dad = static_cast<int>(std::lower_bound(dad->keys.begin(),
                                                          dad->keys.end(),
                                                          key) - dad->keys.begin());

    int k1, k2;

    if (index_in_dad > 0 && dad->children[index_in_dad - 1]->keys.size() > factor_ - 1) {
        auto left_brother = dad->children[index_in_dad - 1];
        k1 = left_brother->keys.back();
        k2 = dad->keys[index_in_dad - 1];

        erase_by_value(node->keys, key);
        node->keys.insert(node->keys.begin(), k2);
        dad->keys[index_in_dad - 1] = k1;
        left_brother->keys.erase(left_brother->keys.end() - 1);
    } else if (index_in_dad < dad->keys.size() && dad->children[index_in_dad + 1]->keys.size() > factor_ - 1) {
        auto right_brother = dad->children[index_in_dad + 1];
        k1 = right_brother->keys.front();
        k2 = dad->keys[index_in_dad];

        erase_by_value(node->keys, key);
        node->keys.insert(node->keys.end(), k2);
        dad->keys[index_in_dad] = k1;
        right_brother->keys.erase(right_brother->keys.begin());
    } else if (index_in_dad > 0) {
        auto new_node = merge(dad->children[index_in_dad - 1],
                              node, dad->keys[index_in_dad - 1]);
        dad->children[index_in_dad - 1] = new_node;
        dad->children.erase(dad->children.begin() + index_in_dad);
        dad->keys.erase(dad->keys.begin() + index_in_dad - 1);
        erase_by_value(new_node->keys, key);
    } else if (index_in_dad < dad->keys.size()) {
        auto new_node = merge(node, dad->children[index_in_dad + 1],
                              dad->keys[index_in_dad]);

        dad->children[index_in_dad] = new_node;
        dad->children.erase(dad->children.begin() + index_in_dad + 1);
        dad->keys.erase(dad->keys.begin() + index_in_dad);
        erase_by_value(new_node->keys, key);
    } else {
        throw std::runtime_error("Не бывает такого что-то пошло не так");
    }
}

void tree::BTree::remove_from_inner(const tree::BTree::NodePtr &node, int key) {
    auto index_of_key = static_cast<int>(std::find(node->keys.begin(),
                                                   node->keys.end(), key) - node->keys.begin());

    if (node->children[index_of_key]->keys.size() > factor_ - 1) {
        auto left_subtree = node->children[index_of_key];
        while (!left_subtree->is_leaf) {
            left_subtree = left_subtree->children.back();
        }
        auto left_k1 = left_subtree->keys.back();
        remove_from_leaf(left_subtree, left_k1);
        node->keys[index_of_key] = left_k1;
    } else if (node->children[index_of_key + 1]->keys.size() > factor_ - 1) {
        auto right_subtree = node->children[index_of_key + 1];
        while (!right_subtree->is_leaf) {
            right_subtree = right_subtree->children.front();
        }
        auto right_k1 = right_subtree->keys.front();
        remove_from_leaf(right_subtree, right_k1);
        node->keys[index_of_key] = right_k1;
    } else {
        auto new_node = merge(node->children[index_of_key],
                              node->children[index_of_key + 1],
                              key);

        node->children[index_of_key] = new_node;
        node->children.erase(node->children.begin() + index_of_key + 1);
        erase_by_value(node->keys, key);
        erase_by_value(new_node->keys, key);

        if (root_ == node && root_->keys.empty()) {
            root_ = new_node;
            root_->parent = nullptr;
        }
    }
}
