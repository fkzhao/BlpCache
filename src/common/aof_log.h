//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include "replica/replication.pb.h"

namespace blp {
    class AOFLog {
    public:
        AOFLog(const std::string& filename);

        void append(uint64_t sequence, const std::string& key, const std::string& value);

        void readAll(std::vector<DataEntry>& entries);

        void readIncrements(uint64_t from_seq, std::vector<DataEntry>& entries);

    private:
        std::string filename_;
        std::mutex mutex_;
    };
}