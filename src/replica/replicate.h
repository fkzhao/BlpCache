//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <brpc/server.h>
#include <brpc/channel.h>

namespace blp {
    class ReplicationClient {
    public:
        static static void startReplica(char* host, uint16_t port);
    };
}