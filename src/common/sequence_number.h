//
// Created by fakzhao on 2025/8/6.
//

#pragma once

#include <atomic>

namespace blp {

    class SequenceNumber {
    public:
        SequenceNumber() : sequence_(0) {}

        // Get the current sequence number
        uint64_t current() const;

        // Increment the sequence number and return the new value
        uint64_t next();

    private:
        std::atomic<uint64_t> sequence_;
    };

} // namespace blp