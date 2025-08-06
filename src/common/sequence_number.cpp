//
// Created by fakzhao on 2025/8/6.
//

#include "sequence_number.h"

namespace blp {
    uint64_t SequenceNumber::current() const {
        return sequence_.load(std::memory_order_relaxed);
    }

    uint64_t SequenceNumber::next() {
        return sequence_.fetch_add(1, std::memory_order_relaxed); // Increment and return the new value
    }
}