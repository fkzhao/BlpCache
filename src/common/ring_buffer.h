//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <vector>
#include <mutex>
#include <optional>
#include <condition_variable>
#include "replica/protocol.h"

namespace blp {
    class RingBuffer {
    public:
        static RingBuffer& getInstance();

        explicit RingBuffer(size_t capacity)
       : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {}

        // Explicitly delete copy constructor and assignment operator
        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        void push(const DataEntity &item);
        std::optional<DataEntity> pop();
        size_t size() const;
        bool empty() const;
    private:
        std::vector<DataEntity> buffer_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        size_t size_;
        mutable std::mutex mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
    };
}