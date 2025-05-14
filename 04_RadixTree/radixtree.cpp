/*
    Implementation of a memory efficient radix tree using value and string pooling
    Could be improved by storing nodes in a Vector in search order (i.e children next to parents) for locality
*/

#include <cassert>
#include <vector>
#include <string_view>
#include <string>
#include <limits>
#include <optional>
#include <stack>
#include <memory>
#include <iostream>
#include <random>
#include <unordered_map>
#include <chrono>
#include <fstream>

template<typename T>
class RadixTree
{
private:
    class StringRef
    {
        static constexpr std::uint32_t MAX_LENGTH = std::numeric_limits<std::uint32_t>::max();
        static constexpr std::size_t INVALID_STORAGE = std::numeric_limits<std::size_t>::max();

    private:
        std::size_t _index;
        std::uint32_t _start;
        std::uint32_t _length;
    
    public:
        StringRef(const std::size_t index,
                  const std::uint32_t start = 0,
                  const std::uint32_t length = MAX_LENGTH) : _index(index),
                                                             _start(start),
                                                             _length(length)
        {}

        StringRef() : _index(INVALID_STORAGE), _start(0), _length(0)
        {}

        bool is_valid() const noexcept { return this->_index != INVALID_STORAGE; }

        std::string_view as_string_view(const std::vector<std::string>& strings) const noexcept
        {
            if(!this->is_valid() || this->_index >= strings.size())
            {
                return std::string_view();
            }

            const std::string& str = strings[this->_index];

            const std::size_t length = (this->_length == MAX_LENGTH) ? 
                str.length() - static_cast<std::size_t>(this->_start) : 
                std::min(static_cast<std::size_t>(this->_length), 
                         str.length() - static_cast<std::size_t>(this->_start));

            return std::string_view(str.data() + this->_start, length);
        }

        StringRef get_sub_ref(const std::uint32_t start, const std::uint32_t length) const noexcept
        {
            if(!this->is_valid())
            {
                return StringRef();
            }

            return StringRef(this->_index, this->_start + start, length);
        }
    };

    struct Node 
    {
        static constexpr std::size_t NO_VALUE = std::numeric_limits<std::size_t>::max();

        std::vector<std::pair<StringRef, Node*>> _children;
        std::size_t _value_index;

        Node() : _value_index(NO_VALUE) {}
        
        explicit Node(std::size_t value_index) : _value_index(value_index) {}

        bool has_value() const noexcept { return this->_value_index != NO_VALUE; }
    };

    std::vector<std::string> _strings;
    std::vector<T> _values;
    std::vector<Node> _nodes;
    Node* _root;

    template<typename S>
    StringRef add_string(S&& str) noexcept
    {
        static_assert(std::is_same_v<std::string, std::remove_reference_t<S>>, 
                      "An std::string must be passed to this function"); 

        this->_strings.emplace_back(std::forward<S>(str));

        return StringRef(this->_strings.size() - 1);
    }

    template<typename... Args>
    std::size_t add_value(Args&&... args) noexcept
    {
        this->_values.emplace_back(std::forward<Args>(args)...);
        return this->_values.size() - 1;
    }

    std::size_t longest_common_prefix_length(const std::string_view lhs, const std::string_view rhs) const noexcept
    {
        const std::size_t min_length = std::min(lhs.length(), rhs.length());

        for(std::size_t i = 0; i < min_length; i++)
        {
            if(lhs[i] != rhs[i])
            {
                return i;
            }
        }

        return min_length;
    }

    template<typename... Args>
    void _insert(std::string key, Args&&... args) noexcept
    {
        const std::size_t value_index = add_value(std::forward<Args>(args)...);

        const StringRef key_ref = this->add_string(std::forward<std::string>(key));
        const std::string_view key_view = key_ref.as_string_view(this->_strings);

        Node* current = this->_root;
        std::size_t key_pos = 0;

        while(key_pos < key_view.length())
        {
            const std::string_view remaining = key_view.substr(key_pos);
            bool found_prefix = false;

            for(auto it = current->_children.begin(); it != current->_children.end();)
            {
                const std::string_view edge = it->first.as_string_view(this->_strings);
                const std::size_t lcpl = this->longest_common_prefix_length(edge, remaining);

                if(lcpl == 0)
                {
                    ++it;
                    continue;
                }

                found_prefix = true;

                if(lcpl == edge.length())
                {
                    current = it->second;
                    key_pos += lcpl;
                    break;
                }

                StringRef prefix_ref = it->first.get_sub_ref(0, lcpl);
                StringRef suffix_ref = it->first.get_sub_ref(lcpl, edge.length() - lcpl);

                Node* new_node = new Node();

                new_node->_children.emplace_back(suffix_ref, it->second);

                auto old_it = it++;
                current->_children.erase(old_it);
                current->_children.emplace_back(prefix_ref, new_node);

                current = new_node;
                key_pos += lcpl;
                break;
            }

            if(!found_prefix)
            {
                StringRef remaining_ref = key_ref.get_sub_ref(key_pos, key_view.length() - key_pos);
                Node* leaf_node = new Node(value_index);
                current->_children.emplace_back(remaining_ref, leaf_node);
                return;
            }
        }

        current->_value_index = value_index;
    }


public:
    RadixTree() : _root(new Node()) {}

    ~RadixTree()
    {
        if(this->_root != nullptr && this->_nodes.size() == 0)
        {
            std::stack<Node*> to_delete;
            to_delete.push(this->_root);

            while(!to_delete.empty())
            {
                Node* current = to_delete.top();
                to_delete.pop();

                for(auto& [_, node] : current->_children)
                {
                    to_delete.push(node);
                }

                delete current;
            }
        }
    }

    RadixTree(const RadixTree&) = delete;
    RadixTree& operator=(const RadixTree&) = delete;
    RadixTree(RadixTree&&) = delete;
    RadixTree& operator=(RadixTree&&) = delete;

    template<typename S, typename... Args,
             typename = std::enable_if_t<!std::is_convertible_v<S, const char*> && 
                                         !std::is_same_v<std::decay_t<S>, std::string>>>
    void insert(S&& key, Args&&... args) noexcept
    {
        static_assert(std::is_same_v<std::decay_t<S>, std::string> || std::is_convertible_v<S, const char*>,
                      "Key must be an std::string, const char* or char*");
    }

    template<typename... Args>
    void insert(std::string key, Args&&... args) noexcept
    {
        if(key.empty())
        {
            return;
        }

        this->_insert(std::move(key), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void insert(const char* key, Args&&... args) noexcept
    {
        if(key == nullptr || key[0] == '\0')
        {
            return;
        }

        this->_insert(std::string(key), std::forward<Args>(args)...);
    }

    std::optional<std::reference_wrapper<T>> find(const std::string& key) noexcept
    {
        if(key.empty())
        {
            return std::nullopt;
        }

        const std::string_view key_view(key);
        Node* current = this->_root;
        std::size_t key_pos = 0;

        while(key_pos < key_view.length())
        {
            const std::string_view remaining = key_view.substr(key_pos);
            bool found = false;

            for(const auto& [edge_ref, child] : current->_children)
            {
                std::string_view edge = edge_ref.as_string_view(this->_strings);
                
                if(remaining.length() >= edge.length() && 
                   remaining.substr(0, edge.length()) == edge)
                {
                    current = child;
                    key_pos += edge.length();
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                return std::nullopt;
            }
        }

        if(current->has_value())
        {
            return std::ref(this->_values[current->_value_index]);
        }
        
        return std::nullopt;
    }

    std::optional<std::reference_wrapper<const T>> cfind(const std::string& key) const noexcept
    {
        if(key.empty())
        {
            return std::nullopt;
        }

        const std::string_view key_view(key);
        const Node* current = this->_root;
        std::size_t key_pos = 0;

        while(key_pos < key_view.length())
        {
            const std::string_view remaining = key_view.substr(key_pos);
            bool found = false;

            for(const auto& [edge_ref, child] : current->_children)
            {
                const std::string_view edge = edge_ref.as_string_view(this->_strings);
                
                if(remaining.length() >= edge.length() && 
                   remaining.substr(0, edge.length()) == edge)
                {
                    current = child;
                    key_pos += edge.length();
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                return std::nullopt;
            }
        }

        if(current->has_value())
        {
            return std::cref(this->_values[current->_value_index]);
        }
        
        return std::nullopt;
    }

    bool contains(const std::string& key) const noexcept
    {
        return cfind(key).has_value();
    }

    void print_tree() const noexcept
    {
        auto print_node = [this](auto&& self, const Node* node, const std::string& prefix, const bool is_root) -> void {
            if(node == nullptr)
            {
                return;
            }

            if(is_root)
            {
                std::cout << "ROOT";
            }

            if(node->has_value())
            {
                std::cout << " -> [VALUE: " << node->_value_index << "]";
            }

            std::cout << "\n";

            for(const auto& [ref, child] : node->_children)
            {
                const std::string_view edge = ref.as_string_view(this->_strings);

                const std::string new_prefix = prefix + " ";
                const std::string edge_string(edge);
                
                std::cout << prefix << "|-- " << edge_string;

                self(self, child, new_prefix, false);
            }
        };

        print_node(print_node, this->_root, "", true);
    }
};

static constexpr std::string_view chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static std::uniform_int_distribution<std::size_t> chars_dis(0, chars.length() - 1);

static std::vector<std::string> words;
static std::uniform_int_distribution<std::size_t> words_dis;

std::string get_random_string(const size_t length, std::mt19937& rng) noexcept
{
    std::string random(length, ' ');

    for(std::size_t i = 0; i < length; i++)
    {
        random[i] = chars[chars_dis(rng)];
    }

    return random;
}

void get_words() noexcept
{
    std::ifstream file;

    try
    {
        file.open("words.txt");
    }
    catch (std::system_error& e)
    {
        std::cerr << "Error while trying to get words from file words.txt:\n";
        std::cerr << e.code().message() << "\n";
        std::exit(e.code().value());
    }

    words.clear();

    std::string line;

    while(std::getline(file, line))
    {
        if(line[0] == '#')
        {
            continue;
        }

        words.emplace_back(std::move(line));
    }

    std::cout << "Obtained " << words.size() << " words from words.txt\n";
}

std::string get_random_word(std::mt19937& rng) noexcept
{
    if(words.size() == 0)
    {
        get_words();
    }

    return words[words_dis(rng)];
}

inline std::vector<std::string> make_random_strings(std::mt19937& rng, const bool use_words = true) noexcept
{
    if(use_words)
    {
        return words;
    }
    else
    {
        std::vector<std::string> strings(words.size());

        std::uniform_int_distribution<std::size_t> length_dis(5, 25);

        for(std::size_t i = 0; i < words.size(); i++)
        {
            strings.emplace_back(std::move(get_random_string(length_dis(rng), rng)));
        }

        return strings;
    }
}

auto get_timepoint() noexcept
{
    return std::chrono::steady_clock::now();
}

template<typename Unit>
double get_duration(const std::chrono::steady_clock::time_point& start) noexcept
{
    return std::chrono::duration_cast<std::chrono::duration<double, Unit>>(std::chrono::steady_clock::now() - start).count();
}

static constexpr std::size_t NUM_RUNS = 10;

int main()
{
    get_words();

    RadixTree<int> tree;
    
    tree.insert("romane", 1);
    tree.insert(std::string("romanus"), 2);
    tree.insert(std::string("romulus"), 3);
    tree.insert(std::string("rubens"), 4);
    tree.insert(std::string("ruber"), 5);
    tree.insert(std::string("rubicon"), 6);
    tree.insert(std::string("rubicundus"), 7);
    
    tree.print_tree();
    
    auto test_lookup = [&tree](const std::string& key) {
        auto result = tree.cfind(key);

        if(result != std::nullopt) 
        {
            std::cout << key << ": " << result->get() << std::endl;
        } 
        else 
        {
            std::cout << key << ": not found" << std::endl;
        }
    };
    
    test_lookup("romane");
    test_lookup("romanus");
    test_lookup("romulus");
    test_lookup("roman");
    test_lookup("ruber");

    /* Benchmark against std::unordered_map */

    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<std::string> strings = make_random_strings(rng);

    std::unordered_map<std::string, std::size_t> bench_map;

    std::size_t map_idx = 0;

    const auto map_emplace_start = get_timepoint();

    for(auto str : strings)
    {
        bench_map.emplace(str, map_idx++);
    }

    std::cout << "Map emplace " << strings.size() << " keys: " << get_duration<std::milli>(map_emplace_start) << " ms\n";

    RadixTree<std::size_t> bench_tree;

    std::size_t tree_idx = 0;

    const auto tree_insert_start = get_timepoint();

    for(auto str : strings)
    {
        bench_tree.insert(str, tree_idx++);
    }

    std::cout << "Tree insert " << strings.size() << " keys: " << get_duration<std::milli>(tree_insert_start) << " ms\n";

    std::shuffle(strings.begin(), strings.end(), rng);

    double map_find_avg = 0.0;
    bool found = false;

    for(std::size_t i = 0; i < NUM_RUNS; i++)
    {
        const auto map_find_start = get_timepoint();

        for(const auto& str : strings)
        {
            found = bench_map.find(str) != bench_map.end();
        }

        const double map_find_duration = get_duration<std::milli>(map_find_start);

        const double t = 1.0 / static_cast<double>(i + 1);
        map_find_avg = (1.0 - t) * map_find_avg + t * map_find_duration;
    }

    std::cout << "Last found: " << found << "\n";
    std::cout << "Map find " << strings.size() << " keys: " << map_find_avg << " ms (" << NUM_RUNS << " runs)\n";

    double tree_contains_avg = 0.0;

    for(std::size_t i = 0; i < NUM_RUNS; i++)
    {
        const auto tree_contains_start = get_timepoint();

        for(const auto& str : strings)
        {
            found = bench_tree.contains(str);
        }

        const double tree_contains_duration = get_duration<std::milli>(tree_contains_start);

        const double t = 1.0 / static_cast<double>(i + 1);
        tree_contains_avg = (1.0 - t) * tree_contains_avg + t * tree_contains_duration;
    }

    std::cout << "Last found: " << found << "\n";
    std::cout << "Tree contains " << strings.size() << " keys: " << tree_contains_avg << " ms (" << NUM_RUNS << " runs)\n";
    
    return 0;
}