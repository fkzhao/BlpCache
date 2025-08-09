//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <vector>
#include <mutex>
#include <optional>

namespace blp {
    template<typename T>
    class RingBuffer {
    public:
        explicit RingBuffer(size_t capacity)
       : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {}

        void push(const T &item);
        std::optional<T> pop();
        size_t size() const;
        bool empty() const;
    private:
        std::vector<T> buffer_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        size_t size_;
        mutable std::mutex mutex_;
    };
}