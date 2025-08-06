//
// Created by fakzhao on 2025/8/6.
//

#pragma once
#include <brpc/server.h>
#include <brpc/channel.h>
#include "replica/replication.pb.h"

namespace blp {
    void StartReplication(const std::string& master_addr);
}