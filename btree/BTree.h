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

    public:

        explicit BTree(int);

        void insert(int);

        void remove(int);
    };

}

#endif //BTREE_H
