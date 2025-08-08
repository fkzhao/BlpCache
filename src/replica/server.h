//
// Created by fakzhao on 2025/8/6.
//

#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <netinet/in.h>
#include <mutex>
#include <condition_variable>
#include "protocol.h"


namespace blp {
    class ReplicationServer {
    public:
        void startServer(uint16_t port) const;
    private:
        std::atomic<bool> running{true};
        std::vector<int> replicas;
        std::mutex replica_mutex;
    };

}