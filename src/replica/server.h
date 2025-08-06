//
// Created by fakzhao on 2025/8/6.
//

#pragma once

#include <brpc/server.h>


#include "replica/replication.pb.h"

namespace blp {
    class ReplicationServiceImpl final : public ReplicationService {
    public:
        ReplicationServiceImpl(const std::string& aof_file_path);
        ~ReplicationServiceImpl();

        // bRPC流接口实现
        void Sync(google::protobuf::RpcController* cntl_base,
                  google::protobuf::Closure* done);

        // 向所有连接推送增量数据
        void PushIncrementalData(const DataEntry& entry);

    private:
        struct ClientStream {
            brpc::StreamId stream_id;
        };

        void BroadcastIncrement(const DataEntry& entry);

        std::string aof_file_path_;
        std::mutex clients_mutex_;
        std::vector<ClientStream> clients_;

        std::atomic<bool> stop_;
    };
}