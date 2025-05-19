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

enum Color : uint32_t 
{
    Black,
    Red,
};

enum Direction : uint32_t
{
    Left,
    Right,
};

template<typename T>
class RedBlackTree
{
    static_assert(has_key<T, std::size_t()>::value, "T Node must have a key() -> std::size_t member function to get the key from which the binary tree will be built");

private:
    struct Node
    {
        Node* _parent;

        Node* _left;
        Node* _right;

        Color _color;

        T _data;

        Node(T&& data, Color color) : _parent(nullptr),
                                      _left(nullptr),
                                      _right(nullptr),
                                      _color(color),
                                      _data(std::forward<T>(data))
        {
        }

        template<typename ...Args>
        Node(Args&&... args) : _parent(nullptr),
                               _left(nullptr),
                               _right(nullptr),
                               _color(Color::Red),
                               _data(std::forward<Args>(args)...)
        {
        }

        inline std::size_t key() const noexcept { return this->_data.key(); }
    };

    Color get_node_color(Node* node) const noexcept { return node == nullptr ? Color::Black : node->_color; }

    Node* _root;

    template<Direction dir>
    Node* rotate(Node* node) noexcept
    {
        Node* parent = node->_parent;
        Node* new_root = dir == Direction::Left ? node->_right : node->_left;
        Node* new_child = dir == Direction::Left ? new_root->_left : new_root->_right;

        if constexpr(dir == Direction::Left)
        {
            node->_right = new_child;
        }
        else
        {
            node->_left = new_child;
        }

        if(new_child != nullptr)
        {
            new_child->_parent = node;
        }

        if constexpr(dir == Direction::Left)
        {
            new_root->_left = node;
        }
        else
        {
            new_root->_right = node;
        }

        new_root->_parent = node->_parent;
        node->_parent = new_root;

        if(parent != nullptr)
        {
            if(node == parent->_right)
            {
                parent->_right = new_root;
            }
            else
            {
                parent->_left = new_root;
            }
        }
        else
        {
            this->_root = new_root;
        }

        return new_root;
    }

public:
    RedBlackTree() : _root(nullptr)
    {
    }

    ~RedBlackTree()
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

        Node* new_node = new Node(std::forward<Args>(args)...);

        Node* current = this->_root;
        Node* parent = nullptr;

        while(current != nullptr)
        {
            parent = current;

            if(new_node->key() < current->key())
            {
                current = current->_left;
            }
            else
            {
                current = current->_right;
            }
        }

        new_node->_parent = parent;

        if(parent == nullptr)
        {
            this->_root = new_node;
            return;
        }
        else if(new_node->key() < parent->key())
        {
            parent->_left = new_node;
        }
        else
        {
            parent->_right = new_node;
        }

        do
        {
            if(this->get_node_color(parent) == Color::Black)
            {
                return;
            }

            Node* grand_parent = parent->_parent;

            if(grand_parent == nullptr)
            {
                parent->_color = Color::Black;
                return;
            }

            if(parent == parent->_parent->_right)
            {
                Node* uncle = grand_parent->_left;
                
                if(this->get_node_color(uncle) == Color::Black)
                {
                    if(new_node == parent->_left)
                    {
                        this->rotate<Direction::Right>(parent);
                        new_node = parent;
                        parent = grand_parent->_right;
                    }

                    this->rotate<Direction::Left>(parent);
                    parent->_color = Color::Black;
                    grand_parent->_color = Color::Red;

                    return;
                }

                parent->_color = Color::Black;
                uncle->_color = Color::Black;
                grand_parent->_color = Color::Red;
                new_node = grand_parent;
            }
            else
            {
                Node* uncle = grand_parent->_right;
                
                if(this->get_node_color(uncle) == Color::Black)
                {
                    if(new_node == parent->_left)
                    {
                        this->rotate<Direction::Right>(parent);
                        new_node = parent;
                        parent = grand_parent->_right;
                    }

                    this->rotate<Direction::Right>(parent);
                    parent->_color = Color::Black;
                    grand_parent->_color = Color::Red;

                    return;
                }

                parent->_color = Color::Black;
                uncle->_color = Color::Black;
                grand_parent->_color = Color::Red;
                new_node = grand_parent;
            }

        } while (parent == new_node->_parent);
    }

    void print() const noexcept
    {
        std::cout << "RedBlackTree:\n";

        auto print_node = [](auto&& self, Node* node, const std::size_t depth) -> void {
            if(node == nullptr)
            {
                return;
            }

            self(self, node->_right, depth + 1);

            std::cout << std::string(depth * 4, ' ') << "Node (" << (node->_color == Color::Red ? "RED" : "BLACK") << "):"  << node->key() << "\n";

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

static constexpr std::size_t NUM_NODES = 10;

int main(int argc, char** argv)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<std::size_t> dis(1, 1000); 

    RedBlackTree<Data> tree;

    for(std::size_t i = 0; i < NUM_NODES; i++)
    {
        tree.insert(dis(rng));
    }

    tree.print();

    return 0;
}