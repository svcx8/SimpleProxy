#ifndef TOKEN_BUCKET_HEADER
#define TOKEN_BUCKET_HEADER

#include <chrono>
#include <stdint.h>
#include <sys/types.h>

#include <absl/status/statusor.h>

#include "dispatcher/ipoller.hh"

class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;
    TokenBucket(const uint64_t fill_peroid_millisecond,
                const uint64_t fill_tick_per_peroid,
                const uint64_t capacity);
    Clock::duration Consume(const uint64_t);
    absl::StatusOr<Clock::duration> Wait(const uint64_t);
    absl::Status GenerateToken(IPoller*, int, Clock::duration);

    bool blocked_ = false;

private:
    const Clock::duration fill_peroid_;
    const uint64_t fill_tick_;
    const uint64_t capacity_;
    int64_t available_;
    Clock::time_point start_time_;
};

#endif // token_bucket.hh