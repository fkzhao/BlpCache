//
// Created by fakzhao on 2025/8/2.
//

#pragma once

#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>

namespace blp {
    void SyncFromMaster(const std::string& master_addr, const std::string& slave_id, int sync_interval_sec);
}