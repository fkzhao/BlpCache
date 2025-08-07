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
    void start_replication_server(uint16_t port);

}