#ifndef TOKEN_BUCKET_HEADER
#define TOKEN_BUCKET_HEADER

#include <chrono>
#include <stdint.h>

class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;
    TokenBucket(const uint64_t capacity,
                const uint64_t fill_tick,
                const uint64_t serving) : capacity_(capacity),
                                          available_(capacity),
                                          fill_tick_(fill_tick),
                                          serving_(serving){};
    void Read(const uint64_t bytes);

private:
    const uint64_t capacity_;
    uint64_t available_;
    const Clock::duration fill_tick_;
    const uint64_t serving_;
};

#endif // token_bucket.hh