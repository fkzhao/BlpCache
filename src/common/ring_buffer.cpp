//
// Created by fakzhao on 2025/8/6.
//

#include "ring_buffer.h"

namespace blp {

    template<typename T>
    void RingBuffer<T>::push(const T &item) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
        if (size_ < capacity_) {
            ++size_;
        } else {
            head_ = (head_ + 1) % capacity_;
        }
    }


    template<typename T>
    std::optional<T> RingBuffer<T>::pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) {
            return std::nullopt;
        }
        T item = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        --size_;
        return item;
    }

    template<typename T>
    size_t RingBuffer<T>::size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    template<typename T>
    bool RingBuffer<T>::empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }

}