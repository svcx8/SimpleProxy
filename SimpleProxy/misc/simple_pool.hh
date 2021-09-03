#ifndef SIMPLE_POOL_HEADER
#define SIMPLE_POOL_HEADER

template <int number, int size>
class SimplePool {
public:
    ~SimplePool() {
        MemoryBlock* next;
        while (memory_block_header_) {
            next = memory_block_header_->next_;
            delete memory_block_header_;
            memory_block_header_ = next;
        }
    }

    void* Allocate() {
        if (!free_chunk_header_) {
            MemoryBlock* new_block = new MemoryBlock;
            free_chunk_header_ = &new_block->chunks_[0];

            for (int i = 1; i < number; ++i) {
                new_block->chunks_[i - 1].next_ = &new_block->chunks_[i];
            }

            if (!memory_block_header_) {
                memory_block_header_ = new_block;
            } else {
                new_block->next_ = memory_block_header_;
                memory_block_header_ = new_block;
            }
        }
        void* allocated_chunk = free_chunk_header_;
        free_chunk_header_ = free_chunk_header_->next_;
        return allocated_chunk;
    }

    void Revert(void* ptr) {
        FreeChunk* chunk = (FreeChunk*)ptr;
        chunk->next_ = free_chunk_header_;
        free_chunk_header_ = chunk;
    }

private:
    class FreeChunk {
    public:
        FreeChunk* next_ = nullptr;
        char data_[size];
    };
    FreeChunk* free_chunk_header_ = nullptr;

    class MemoryBlock {
    public:
        MemoryBlock* next_ = nullptr;
        FreeChunk chunks_[number];
    };
    MemoryBlock* memory_block_header_ = nullptr;
};

#endif // simple_pool.hh