#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <memory>
#include <stack>

namespace tree {
    class BTree {
    private:
        struct Node {
            int factor;
            std::vector<int> keys;
            std::vector<std::shared_ptr<Node>> children;
            std::shared_ptr<Node> parent;
            bool is_leaf;

            explicit Node(int);

            [[nodiscard]] bool is_full() const;
        };

        using NodePtr = std::shared_ptr<Node>;

        int factor_;
        NodePtr root_;

        void split(const NodePtr &);

        NodePtr merge(const NodePtr &, const NodePtr &, int);

        bool move_key(const NodePtr &);

        void remove_from_leaf(const NodePtr &, int);

        void remove_from_inner(const NodePtr &, int);

        struct Iterator {
        public:
            Iterator(std::stack<int> &, NodePtr);

            int operator*() const;

            Iterator &operator++();

            bool operator==(const Iterator &) const;

            bool operator!=(const Iterator &) const;

        private:
            std::stack<int> indexes_path_;
            NodePtr current_node_;
            bool return_from_child_just_now_;
        };

    public:

        explicit BTree(int);

        void insert(int);

        void remove(int);

        Iterator begin();
        Iterator end();
    };
}

#endif //BTREE_H
