#ifndef UDP_BUFFER_HEADER
#define UDP_BUFFER_HEADER

#include <sys/socket.h>
#include <unordered_map>

#include <absl/status/status.h>

class UDPBuffer {
public:
    /*
        FreeChunk* Allocate();

        class FreeChunk {
            public:
                FreeChunk* next_ = nullptr;
                char data_[size];
            };
        }

        static_cast<MemoryBuffer*>(Allocate());

        // ~~~I don't know why it is worked before, but this is definitely a BUG.~~~
        // Not a bug, just a feature.
    */
    constexpr static int buffer_size_ = 64 * 1024; // A theoretical limit for a UDP datagram, google tell me this.
    unsigned char buffer_[buffer_size_];           // -> FreeChunk* next_, that's ok because no more read it.
    int start_;                                    // -> store in &buffer_[sizeof(next_)]
    int end_;                                      // -> store in &buffer_[sizeof(next_) + sizeof(start_)]

    sockaddr_storage client_addr_;

    int Usage() {
        return end_ - start_;
    }

    absl::Status Receive(int s);
    absl::Status Send(int s, void* client, int client_len, int offset);

    static UDPBuffer* GetPool(int s);
    static void RemovePool(int s);

    static std::unordered_map<int, UDPBuffer*> buffer_array_;
};

#endif // udp_buffer.hh