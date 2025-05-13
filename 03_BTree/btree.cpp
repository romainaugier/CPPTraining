/*
    Binary search tree
*/

#include <type_traits>
#include <stack>
#include <iostream>
#include <string>
#include <random>

template<typename, typename T>
struct has_key {
    static_assert(
        std::integral_constant<T, false>::value,
        "Second template parameter needs to be of function type.");
};

template<typename C, typename Ret, typename... Args>
struct has_key<C, Ret(Args...)> 
{
private:
    template<typename T>
    static constexpr auto check(T*)
    -> typename
        std::is_same<
            decltype( std::declval<T>().key( std::declval<Args>()... ) ),
            Ret
        >::type;

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(0)) type;

public:
    static constexpr bool value = type::value;
};

template<typename T>
class BinaryTree
{
    static_assert(has_key<T, std::size_t()>::value, "T Node must have a key() -> std::size_t member function to get the key from which the binary tree will be built");

private:
    struct Node
    {
        Node* _parent;

        Node* _left;
        Node* _right;

        T _data;

        Node(T&& data) : _parent(nullptr),
                         _left(nullptr),
                         _right(nullptr),
                         _data(std::forward<T>(data))
        {
        }

        template<typename ...Args>
        Node(Args&&... args) : _parent(nullptr),
                            _left(nullptr),
                            _right(nullptr)
        {
            ::new(std::addressof(this->_data)) T(std::forward<Args>(args)...);
        }

        inline std::size_t key() const noexcept { return this->_data.key(); }
    };

    Node* _root;

public:
    BinaryTree() : _root(nullptr)
    {
    }

    ~BinaryTree()
    {
        if(this->_root != nullptr)
        {
            std::stack<Node*> to_delete;
            to_delete.push(this->_root);

            while(!to_delete.empty())
            {
                Node* current = to_delete.top();
                to_delete.pop();

                if(current->_left != nullptr)
                {
                    to_delete.push(current->_left);
                }

                if(current->_right != nullptr)
                {
                    to_delete.push(current->_right);
                }

                delete current;
            }
        }
    }

    template<typename ...Args>
    void insert(Args&&... args) noexcept
    {
        T data(std::forward<Args>(args)...);

        auto insert_node = [&](auto&& self, Node* node) -> Node* {
            if(node == nullptr)
            {
                return new Node(std::move(data));
            }

            if(node->key() == data.key())
            {
                return node;
            }

            if(node->key() < data.key())
            {
                node->_right = self(self, node->_right);
            }
            else
            {
                node->_left = self(self, node->_left);
            }

            return node;
        };

        this->_root = insert_node(insert_node, this->_root);
    }

    void print() const noexcept
    {
        std::cout << "BTree:\n";

        auto print_node = [](auto&& self, Node* node, const std::size_t depth) -> void {
            if(node == nullptr)
            {
                return;
            }

            self(self, node->_right, depth + 1);

            std::cout << std::string(depth * 4, ' ') << "Node: " << node->key() << "\n";

            self(self, node->_left, depth + 1);
        };

        print_node(print_node, this->_root, 0);
    }
};

class Data
{
private:
    std::size_t _key;

public:
    Data(const std::size_t key) : _key(key) {}

    std::size_t key() const noexcept { return this->_key; }
};

static constexpr std::size_t NUM_NODES = 100;

int main(int argc, char** argv)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<std::size_t> dis(1, 1000); 

    BinaryTree<Data> tree;

    for(std::size_t i = 0; i < NUM_NODES; i++)
    {
        tree.insert(dis(rng));
    }

    tree.print();

    return 0;
}