//
// Created by fakzhao on 2025/8/6.
//

#include "ring_buffer.h"

namespace blp {
    static std::unique_ptr<RingBuffer> instance_;
    RingBuffer& RingBuffer::getInstance() {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            instance_ = std::unique_ptr<RingBuffer>(new RingBuffer(1024));
        });
        return *instance_;
    }

    void RingBuffer::push(const DataEntity &item) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
        if (size_ < capacity_) {
            ++size_;
        } else {
            head_ = (head_ + 1) % capacity_;
        }
        not_empty_.notify_one();
    }


    std::optional<DataEntity> RingBuffer::pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) {
            return std::nullopt; // or throw an exception
        }
        DataEntity item = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        --size_;
        not_empty_.notify_one();
        return item;
    }

    size_t RingBuffer::size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    bool RingBuffer::empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }

}