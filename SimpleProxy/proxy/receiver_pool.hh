#ifndef RECEIVER_POOL_HEADER
#define RECEIVER_POOL_HEADER

#include <misc/singleton.hh>

#include <map>
#include <vector>

class ReceiverPool : public Singleton<ReceiverPool> {
public:
    bool used_ = false;
    unsigned char* buffer_;
    int start_ = 0;
    int end_ = 0;
    int Usage() {
        return end_ - start_;
    }
    void Clear(int s);
    int Transfer(int s);
    unsigned char* GetPtr() {
        return &buffer_[end_];
    }
    static int buffer_size_;
    static void Init(int pool_size, int buffer_size);
    static ReceiverPool* GetPool(int s);
    static void Remove(int s);

private:
    ReceiverPool* GetBufferPoll();
    std::map<int, ReceiverPool*> receiver_array_;
    std::vector<ReceiverPool*> buffer_array_;
};

#endif // receiver_pool.hh