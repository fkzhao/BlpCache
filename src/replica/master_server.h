//
// Created by fakzhao on 2025/8/2.
//

#pragma once

#include "sync_service.pb.h"
#include <brpc/server.h>

namespace blp {
    class MasterServer : public SyncService {
        void SyncData(
            google::protobuf::RpcController *cntl_base,
            const SyncRequest *request, SyncResponse *response,
            google::protobuf::Closure *done);
    };
}