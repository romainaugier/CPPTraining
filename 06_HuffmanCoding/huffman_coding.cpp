/*
    Implementation of Huffman encoding/decoding for strings
*/

#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <stack>
#include <iostream>
#include <numeric>

class Huffman
{
private:
    struct Node 
    {
        static constexpr char NO_SYMBOL = 0;
        static constexpr std::size_t NO_NODE = std::numeric_limits<std::size_t>::max();

        std::size_t _left;
        std::size_t _right;

        std::size_t _frequency;

        char _symbol;

        char _padding[7]; /* For 32 bytes size */

        Node() : _left(NO_NODE),
                 _right(NO_NODE),
                 _frequency(0),
                 _symbol(NO_SYMBOL)
        {}

        Node(char symbol) : _left(NO_NODE),
                            _right(NO_NODE),
                            _frequency(0),
                            _symbol(symbol)
        {}

        Node(std::size_t left, std::size_t right, std::uint32_t frequency) : _left(left),
                                                                             _right(right),
                                                                             _frequency(frequency),
                                                                             _symbol(NO_SYMBOL)
        {}

        bool is_leaf() const noexcept { return this->_left == NO_NODE && this->_right == NO_NODE; }
    };

public:
    static std::string encode(const std::string& str) noexcept
    {
        std::vector<Node> nodes;
        std::unordered_map<char, std::string> codes;
        std::size_t root = Node::NO_NODE;

        std::unordered_map<char, std::size_t> char_mapping;

        for(const char c : str)
        {
            if(char_mapping.find(c) == char_mapping.end())            
            {
                nodes.emplace_back(c);
                char_mapping[c] = nodes.size() - 1;
            }

            nodes[char_mapping[c]]._frequency++;
        }

        auto comp = [&](const std::size_t lhs, const std::size_t rhs) { return nodes[lhs]._frequency > nodes[rhs]._frequency; };

        std::vector<std::size_t> node_indices(nodes.size());
        std::iota(node_indices.begin(), node_indices.end(), 0);

        std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(comp)> queue(node_indices.begin(), node_indices.end(), comp);

        while(!queue.empty())
        {
            if(queue.size() == 1)
            {
                root = queue.top();
                break;
            }

            std::size_t l = queue.top();
            queue.pop();

            std::size_t r = queue.top();
            queue.pop();

            nodes.emplace_back(l, r, nodes[l]._frequency + nodes[r]._frequency);

            queue.push(nodes.size() - 1);
        }

        auto traverse = [&](auto&& self, std::size_t node_index, std::string code) -> void {
            if(node_index == Node::NO_NODE || node_index >= nodes.size())
            {
                return;
            }

            const Node& node = nodes[node_index];

            if(node.is_leaf())
            {
                codes[node._symbol] = std::move(code);
                return;
            }

            self(self, node._left, code + "0");
            self(self, node._right, code + "1");
        };

        traverse(traverse, root, "");

        std::cout << str << "\n";
        std::cout << "Huffman codes:\n";

        for(const auto& [symbol, code] : codes)
        {
            std::cout << symbol << " " << code << "\n";
        }

        std::string encoded;

        for(const auto& c : str)
        {
            encoded += codes[c];
        }

        return encoded;
    }
};

int main(int argc, char** argv) noexcept
{
    const std::string str = "HUFFMAN";

    std::cout << Huffman::encode(str) << "\n";

    return 0;
}