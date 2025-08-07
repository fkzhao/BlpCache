//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <brpc/server.h>
#include <brpc/channel.h>

namespace blp {
    void start_replica(char* host, uint16_t port);
}