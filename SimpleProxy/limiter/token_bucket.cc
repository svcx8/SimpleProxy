#include "token_bucket.hh"

#include <linux/eventpoll.h>
#include <sys/types.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include <absl/status/status.h>

#include "misc/logger.hh"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

TokenBucket::TokenBucket(const uint64_t fill_peroid_millisecond,
                         const uint64_t fill_tick_per_peroid,
                         const uint64_t capacity) : fill_peroid_(duration_cast<Clock::duration>(milliseconds(fill_peroid_millisecond))),
                                                    fill_tick_(fill_tick_per_peroid),
                                                    capacity_(capacity),
                                                    available_(capacity),
                                                    start_time_(Clock::now()) {
    LOG("TokenBucket: Fill %lu ticks per %lu ms, capacity: %lu", fill_tick_per_peroid, fill_peroid_millisecond, capacity);
}

absl::Status TokenBucket::GenerateToken(IPoller* poller, int s, Clock::duration sleep_time) {
    blocked_ = true;
    auto result = poller->RemoveSocket(s);
    if (result.ok()) {
        LOG("[%d] [TokenBucket::GenerateToken] Sleep %llu ms", s, duration_cast<milliseconds>(sleep_time).count());
        std::this_thread::sleep_for(sleep_time);
        result.Update(poller->AddSocket(s, EPOLLIN));
    }
    blocked_ = false;
    return result;
}

auto TokenBucket::Wait(const uint64_t count) -> absl::StatusOr<Clock::duration> {
    if (blocked_) {
        return absl::InternalError("Disabled token bucket called.");
    }
    return Consume(count);
}

auto TokenBucket::Consume(const uint64_t consume_bytes) -> Clock::duration {
    auto cycles = (Clock::now() - start_time_) / fill_peroid_;

    auto filled_token = cycles * fill_tick_;

    available_ += filled_token;
    if (available_ > capacity_) {
        available_ = capacity_;
    }

    LOG("\n[TokenBucket] Consume %lu bytes, add %lld tokens, available: %ld", consume_bytes, filled_token, available_);

    int64_t remain = available_ - consume_bytes;

    if (remain >= 0) {
        available_ = remain;
        return Clock::duration(0);
    }

    auto wait_ticks = (-remain + fill_tick_ - 1) / fill_tick_;
    available_ = remain;
    return wait_ticks * fill_peroid_;
}