/*
    Memory allocator implementation using Red-Black Tree, not thread-safe
*/

#include <cstdint>
#include <cstddef>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <cmath>

enum Color : std::size_t {
    Red,
    Black,
};

enum BlockState : std::size_t {
    Free,
    Occupied,
};

inline void* align_forward(const void* ptr, const std::size_t alignment) noexcept
{
    const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
    const std::uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    return reinterpret_cast<void*>(aligned_addr);
}

class Allocator
{
private:
    static constexpr std::size_t MIN_BLOCK_SIZE = 32;

    enum class Direction {
        Left,
        Right,
    };

    /* Each block should be 64 bytes, i.e cache-line size */
    class MemoryBlock
    {
        friend class Allocator;

    private:
        MemoryBlock* _parent;

        MemoryBlock* _left;
        MemoryBlock* _right;

        MemoryBlock* _next;
        MemoryBlock* _prev;

        void* _address;

        std::size_t _size;
        std::size_t _alignment : 62;

        std::size_t _color : 1;
        std::size_t _state : 1;

    public:
        MemoryBlock(const std::size_t size, 
                    void* address,
                    const std::size_t alignment = 0) : _size(size),
                                                       _address(address), 
                                                       _alignment(alignment),
                                                       _parent(nullptr),
                                                       _left(nullptr),
                                                       _right(nullptr),
                                                       _next(nullptr),
                                                       _prev(nullptr),
                                                       _color(Color::Red),
                                                       _state(BlockState::Free)
        {
        }
    };

    MemoryBlock* _root;

    std::size_t _size;

    std::size_t _num_blocks;

    void* _base_address;

    template<Direction Dir>
    void rotate(MemoryBlock* block) noexcept
    {
        static_assert(Dir == Direction::Left || Dir == Direction::Right, "Dir template parameter must be Direction::Left or Direction::Right");

        MemoryBlock* child = nullptr;
        MemoryBlock*& block_child_ptr = (Dir == Direction::Left) ? block->_right : block->_left;

        child = block_child_ptr;

        if(child == nullptr) 
        {
            return;
        }

        block_child_ptr = (Dir == Direction::Left) ? child->_left : child->_right;

        if(block_child_ptr != nullptr) 
        {
            block_child_ptr->_parent = block;
        }

        child->_parent = block->_parent;

        if(block->_parent == nullptr) 
        {
            this->_root = child;
        } 
        else if (block == block->_parent->_left) 
        {
            block->_parent->_left = child;
        } 
        else 
        {
            block->_parent->_right = child;
        }

        if constexpr (Dir == Direction::Left) 
        {
            child->_left = block;
        } 
        else 
        {
            child->_right = block;
        }

        block->_parent = child;
    }

    void fixInsert(MemoryBlock* block) noexcept
    {
        MemoryBlock* parent = nullptr;
        MemoryBlock* grandparent = nullptr;

        while(block != this->_root && block->_parent->_color == Color::Red)
        {
            parent = block->_parent;
            grandparent = parent->_parent;

            if(grandparent == nullptr)
            {
                break;
            }

            if(parent == grandparent->_left)
            {
                MemoryBlock* uncle = grandparent->_right;

                if(uncle != nullptr && uncle->_color == Color::Red)
                {
                    parent->_color = Color::Black;
                    uncle->_color = Color::Black;
                    grandparent->_color = Color::Red;
                    block = grandparent;
                }
                else
                {
                    if(block == parent->_right)
                    {
                        block = parent;
                        this->rotate<Direction::Left>(block);
                        parent = block->_parent;
                    }

                    parent->_color = Color::Black;
                    grandparent->_color = Color::Red;
                    this->rotate<Direction::Right>(grandparent);
                }
            }
            else
            {
                MemoryBlock* uncle = grandparent->_left;

                if(uncle != nullptr && uncle->_color == Color::Red)
                {
                    parent->_color = Color::Black;
                    uncle->_color = Color::Black;
                    grandparent->_color = Color::Red;
                    block = grandparent;
                }
                else
                {
                    if(block == parent->_left)
                    {
                        block = parent;
                        this->rotate<Direction::Right>(block);
                        parent = block->_parent;
                    }

                    parent->_color = Color::Black;
                    grandparent->_color = Color::Red;
                    this->rotate<Direction::Left>(grandparent);
                }
            }
        }

        this->_root->_color = Color::Black;
    }

    void insert(MemoryBlock* block) noexcept
    {
        MemoryBlock* current = this->_root;
        MemoryBlock* parent = nullptr;

        while(current != nullptr)
        {
            parent = current;

            if(block->_size < current->_size)
            {
                current = current->_left;
            }
            else
            {
                current = current->_right;
            }
        }

        block->_parent = parent;

        if(parent == nullptr)
        {
            this->_root = block;
        }
        else if(block->_size < parent->_size)
        {
            parent->_left = block;
        }
        else
        {
            parent->_right = block;
        }

        block->_left = nullptr;
        block->_right = nullptr;
        block->_color = Color::Red;

        this->fixInsert(block);

        this->_num_blocks++;
    }

    void transplant(MemoryBlock* lhs, MemoryBlock* rhs) noexcept
    {
        if(lhs->_parent == nullptr)
        {
            this->_root = rhs;
        }
        else if(lhs == lhs->_parent->_left)
        {
            lhs->_parent->_left = rhs;
        }
        else
        {
            lhs->_parent->_right = rhs;
        }

        if(rhs != nullptr)
        {
            rhs->_parent = lhs->_parent;
        }
    }

    MemoryBlock* minimum(MemoryBlock* block) const noexcept
    {
        while(block != nullptr && block->_left != nullptr)
        {
            block = block->_left;
        }

        return block;
    }

    MemoryBlock* maximum(MemoryBlock* block) const noexcept
    {
        while(block != nullptr && block->_right != nullptr)
        {
            block = block->_right;
        }

        return block;
    }

    void fix_remove(MemoryBlock* block) noexcept
    {
        while(block != this->_root && block != nullptr && this->_root->_color == Color::Black)
        {
            if(block == block->_parent->_left)
            {
                MemoryBlock* sib = block->_parent->_right;

                if(sib == nullptr)
                {
                    break;
                }

                if(sib->_color == Color::Red)
                {
                    sib->_color = Color::Black;
                    block->_parent->_color = Color::Red;

                    this->rotate<Direction::Left>(block->_parent);

                    sib = block->_parent->_right;
                }

                if((sib->_left == nullptr || sib->_left->_color == Color::Black) && (sib->_right == nullptr || sib->_right->_color == Color::Black))
                {
                    sib->_color = Color::Red;
                    block = block->_parent;
                }
                else
                {
                    if(sib->_right == nullptr || sib->_right->_color == Color::Black)
                    {
                        if(sib->_left != nullptr)
                        {
                            sib->_left->_color = Color::Black;
                        }

                        sib->_color = Color::Red;

                        this->rotate<Direction::Right>(sib);

                        sib = block->_parent->_right;
                    }

                    sib->_color = block->_parent->_color;
                    block->_parent->_color = Color::Black;
                    
                    if(sib->_right != nullptr)
                    {
                        sib->_right->_color = Color::Black;
                    }

                    this->rotate<Direction::Left>(block->_parent);

                    block = this->_root;
                }
            }
            else
            {
                MemoryBlock* sib = block->_parent->_left;

                if(sib == nullptr)
                {
                    break;
                }

                if(sib->_color == Color::Red)
                {
                    sib->_color = Color::Black;
                    block->_parent->_color = Color::Red;

                    this->rotate<Direction::Right>(block->_parent);

                    sib = block->_parent->_left;
                }

                if((sib->_right == nullptr || sib->_right->_color == Color::Black) && (sib->_left == nullptr || sib->_left->_color == Color::Black))
                {
                    sib->_color = Color::Red;
                    block = block->_parent;
                }
                else
                {
                    if(sib->_left == nullptr || sib->_left->_color == Color::Black)
                    {
                        if(sib->_right != nullptr)
                        {
                            sib->_right->_color = Color::Black;
                        }

                        sib->_color = Color::Red;

                        this->rotate<Direction::Left>(sib);

                        sib = block->_parent->_left;
                    }

                    sib->_color = block->_parent->_color;
                    block->_parent->_color = Color::Black;
                    
                    if(sib->_left != nullptr)
                    {
                        sib->_left->_color = Color::Black;
                    }

                    this->rotate<Direction::Right>(block->_parent);

                    block = this->_root;
                }
            }
        }

        if(block != nullptr)
        {
            block->_color = Color::Black;
        }
    }

    void remove(MemoryBlock* block) noexcept
    {
        MemoryBlock* x = nullptr;
        MemoryBlock* y = block;
        std::size_t originalColor = y->_color;
        
        if(block->_left == nullptr) 
        {
            x = block->_right;
            transplant(block, block->_right);
        }
        else if(block->_right == nullptr) 
        {
            x = block->_left;
            transplant(block, block->_left);
        }
        else 
        {
            y = minimum(block->_right);
            originalColor = y->_color;
            x = y->_right;
            
            if(y->_parent == block) 
            {
                if(x != nullptr)
                {
                    x->_parent = y;
                }
            } 
            else 
            {
                transplant(y, y->_right);
                y->_right = block->_right;

                if(y->_right != nullptr)
                {
                    y->_right->_parent = y;
                }
            }
            
            transplant(block, y);
            y->_left = block->_left;
            y->_left->_parent = y;
            y->_color = block->_color;
        }
        
        if(originalColor == Color::Black)
        {
            this->fix_remove(x);
        }

        this->_num_blocks--;
    }

    MemoryBlock* find_best_fit(const std::size_t size, MemoryBlock* block) const noexcept
    {
        if(block == nullptr)
        {
            return nullptr;
        }

        if(block->_state == BlockState::Free && block->_size >= size)
        {
            MemoryBlock* left = this->find_best_fit(size, block->_left);

            if(left != nullptr)
            {
                return left;
            }

            return block;
        }

        if(block->_size < size)
        {
            return this->find_best_fit(size, block->_right);
        }

        MemoryBlock* left = this->find_best_fit(size, block->_left);

        if(left != nullptr)
        {
            return left;
        }

        return this->find_best_fit(size, block->_right);
    }

    void split_block(MemoryBlock* block, const std::size_t requested_size) noexcept
    {
        const std::size_t original_usable_size = block->_size;
        constexpr std::size_t header_size = sizeof(MemoryBlock);
        constexpr std::size_t block_alignment = alignof(MemoryBlock);

        const void* data_end = static_cast<char*>(block->_address) + requested_size;

        void* new_header_addr = align_forward(data_end, block_alignment);

        const std::size_t padding = reinterpret_cast<std::uintptr_t>(new_header_addr) - reinterpret_cast<std::uintptr_t>(data_end);

        void* new_data_addr = static_cast<char*>(new_header_addr) + header_size;

        const std::size_t first_chunk_actual_size = requested_size + padding;
        const std::size_t required_for_second_chunk = header_size + MIN_BLOCK_SIZE;

        if (original_usable_size >= first_chunk_actual_size + required_for_second_chunk)
        {
            const std::size_t new_usable_size = original_usable_size - first_chunk_actual_size - header_size;

            MemoryBlock* new_block = new(new_header_addr) MemoryBlock(new_usable_size, new_data_addr);

            new_block->_prev = block;
            new_block->_next = block->_next;

            if(block->_next != nullptr) 
            {
                block->_next->_prev = new_block;
            }

            block->_next = new_block;
            block->_size = requested_size;

            this->insert(new_block);
        }
    }

    void merge_blocks(MemoryBlock* block) noexcept
    {
        if(block->_next && block->_next->_state == BlockState::Free) 
        {
            MemoryBlock* nextBlock = block->_next;
            
            this->remove(block);
            this->remove(nextBlock);
            
            block->_size += sizeof(MemoryBlock) + nextBlock->_size;
            
            block->_next = nextBlock->_next;

            if(nextBlock->_next) 
            {
                nextBlock->_next->_prev = block;
            }
            
            this->insert(block);
        }
        
        if(block->_prev && block->_prev->_state == BlockState::Free) 
        {
            MemoryBlock* prevBlock = block->_prev;
            
            this->remove(prevBlock);
            this->remove(block);
            
            prevBlock->_size += sizeof(MemoryBlock) + block->_size;
            
            prevBlock->_next = block->_next;

            if(block->_next) 
            {
                block->_next->_prev = prevBlock;
            }
            
            this->insert(prevBlock);
        }
    }

    MemoryBlock* address_to_block(void* ptr) const noexcept
    {
        if(ptr == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<MemoryBlock*>(static_cast<char*>(ptr) - sizeof(MemoryBlock));
    }

public:
    Allocator(const std::size_t size) : _root(nullptr), 
                                        _size(size),
                                        _base_address(nullptr),
                                        _num_blocks(0)
    {
        /* TODO: implement memory allocation here */ 

        this->_base_address = std::malloc(size);

        MemoryBlock* initial_block = new (this->_base_address) MemoryBlock(size - sizeof(MemoryBlock), 
                                                                           static_cast<void*>(static_cast<char*>(this->_base_address) + sizeof(MemoryBlock)));

        this->insert(initial_block);
    }

    ~Allocator()
    {
        if(this->_base_address != nullptr)
        {
            std::free(this->_base_address);
            this->_base_address = nullptr;
        }
    }

    void* alloc(const std::size_t size, const std::size_t alignment) noexcept
    {
        MemoryBlock* best = this->find_best_fit(size, this->_root);

        if(best == nullptr)
        {
            return nullptr;
        }

        if(best->_size > (size + sizeof(MemoryBlock) + 32))
        {
            this->split_block(best, size);
        }

        best->_state = BlockState::Occupied;

        return best->_address;
    }

    void free(void* ptr) noexcept
    {
        if(ptr == nullptr)
        {
            return;
        }

        MemoryBlock* block = this->address_to_block(ptr);

        if(block == nullptr)
        {
            return;
        }

        block->_state = BlockState::Free;

        this->merge_blocks(block);
    }

    void print() const noexcept 
    {
        std::cout << "Allocator Tree (blocks: " << this->_num_blocks << "):\n";

        auto print_block = [](auto&& self, MemoryBlock* block, const int depth) -> void {
            if(block == nullptr)
            {
                return;
            }

            self(self, block->_right, depth + 1);

            std::string indent(depth * 4, ' ');
            std::string color = block->_color == Color::Red ? "RED" : "BLACK";
            std::string state = block->_state == BlockState::Free ? "FREE" : "USED";

            std::cout << indent << "+" << block->_size << " bytes [" << color << ", " << state << "] @ " << block->_address << "\n";

            self(self, block->_left, depth + 1);
        };

        print_block(print_block, this->_root, 0);
    }
};

int main(int argc, char** argv)
{
    Allocator allocator(16384);

    for(std::uint32_t i = 1; i < 16; i++)
    {
        void* addr = allocator.alloc(i * 4, 32);

        if(i % 4 == 0)
        {
            allocator.free(addr);
        }
    }

    allocator.print();

    return 0;
}