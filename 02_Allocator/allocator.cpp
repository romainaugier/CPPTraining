/*
    Memory allocator implementation using Red-Black Tree, not thread-safe
*/

#include <cstdint>
#include <cstddef>
#include <memory>

enum class Color {
    Red,
    Black,
};

enum class BlockState {
    Free,
    Occupied,
};

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

        std::size_t _size;
        void* _address;
        std::size_t _alignment : 62;

        Color _color : 1;
        BlockState _state : 1;

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

    template<Direction Dir>
    void rotate(MemoryBlock* block) noexcept
    {
        static_assert(Dir == Direction::Left || Dir == Direction::Right, "Dir template parameter must be Direction::Left or Direction::Right");

        MemoryBlock* child;

        if constexpr (Dir == Direction::Left)
        {
            child = block->_right;
            block->_right = child->_left;

            if(block->_right != nullptr)
            {
                block->_right->_parent = block;
            }
        }
        else
        {
            child = block->_left;
            block->_left = child->_right;

            if(block->_left != nullptr)
            {
                block->_left->_parent = block;
            }
        }

        if(block->_parent == nullptr)
        {
            this->_root = child;
        }
        else if(block == block->_parent->_left)
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

    void fixRemove(MemoryBlock* block) noexcept
    {
        while(block != this->_root && block != nullptr && this->_root->_color == Color::Black)
        {
            if(block == block->_parent->_left)
            {
                MemoryBlock* sib = block->_parent->_right;

                if(sib != nullptr && sib->_color == Color::Red)
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

                if(sib != nullptr && sib->_color == Color::Red)
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
        Color originalColor = y->_color;
        
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
            this->fixRemove(x);
        }
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

    void split_block(MemoryBlock* block, const std::size_t size) noexcept
    {
        const std::size_t remaining_size = block->_size - size - sizeof(MemoryBlock);

        if(remaining_size >= MIN_BLOCK_SIZE)
        {
            void* address = static_cast<void*>(static_cast<char*>(block->_address) + size);

            MemoryBlock* new_block = new(address) MemoryBlock(remaining_size, static_cast<void*>(static_cast<char*>(address) + sizeof(MemoryBlock)));

            new_block->_next = block->_next;
            new_block->_prev = block;

            if(block->_next != nullptr)
            {
                block->_next->_prev = new_block;
            }

            block->_next = new_block;

            block->_size = size;

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
    Allocator(std::size_t size) : _root(nullptr)
    {
        /* TODO: implement memory allocation here */ 
    }

    ~Allocator()
    {

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

        this->remove(best);

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

        this->insert(block);

        this->merge_blocks(block);
    }
};

int main(int argc, char** argv)
{
    return 0;
}