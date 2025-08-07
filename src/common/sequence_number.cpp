//
// Created by fakzhao on 2025/8/6.
//

#include "sequence_number.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace blp {
    uint64_t SequenceNumber::current() const {
        return sequence_.load(std::memory_order_relaxed);
    }

    uint64_t SequenceNumber::next() {
        return sequence_.fetch_add(1, std::memory_order_relaxed); // Increment and return the new value
    }

    std::string SequenceNumber::next_as_string() {
        uint64_t value = sequence_.fetch_add(1, std::memory_order_relaxed); // Increment and get the new value
        std::ostringstream oss;
        oss << std::setw(20) << std::setfill('0') << value;
        return std::to_string(value);
    }
}