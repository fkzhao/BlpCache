//
// Created by fakzhao on 2025/8/6.
//

#pragma once

#include <iostream>
#include <atomic>
#include <sstream>

namespace blp {

    static uint64_t seq_to_uint64_t(const std::string &str) {
        if (str.length() > 20) {
            throw std::invalid_argument("Input string exceeds 20 digits");
        }
        for (char c : str) {
            if (!isdigit(c)) {
                throw std::invalid_argument("Input string contains non-digit characters");
            }
        }

        uint64_t value = 0;
        std::istringstream iss(str);
        iss >> value;
        if (iss.fail()) {
            throw std::invalid_argument("Conversion failed");
        }
        return value;
    }

    class SequenceNumber {
    public:
        SequenceNumber() : sequence_(0) {}

        // Get the current sequence number
        uint64_t current() const;

        // Increment the sequence number and return the new value
        uint64_t next();

        std::string next_as_string();

    private:
        std::atomic<uint64_t> sequence_;
    };

} // namespace blp