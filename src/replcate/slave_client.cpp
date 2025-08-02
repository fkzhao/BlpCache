//
// Created by fakzhao on 2025/8/2.
//

#include "slave_client.h"
#include <brpc/channel.h>
#include "sync_service.pb.h"
#include <iostream>
#include <unistd.h>
#include <leveldb/write_batch.h>

#include "core/db.h"

namespace blp {
    auto db = LevelDBWrapper::getInstance();
    void SyncFromMaster(const std::string& master_addr, const std::string& slave_id, int sync_interval_sec) {
        brpc::Channel channel;
        brpc::ChannelOptions options;
        options.protocol = "baidu_std";
        options.timeout_ms = 5000;
        if (channel.Init(master_addr.c_str(), &options) != 0) {
            std::cerr << "Fail to init channel" << std::endl;
            return;
        }

        SyncService_Stub stub(&channel);
        std::string last_message_id = "";
        bool full_sync_done = false;

        while (true) {
            std::string next_message_id = last_message_id;
            bool has_more = true;

            while (has_more) {
                SyncRequest request;
                request.set_slave_id(slave_id);
                if (!next_message_id.empty()) {
                    request.set_last_message_id(next_message_id);
                }

                SyncResponse response;
                brpc::Controller cntl;
                stub.SyncData(&cntl, &request, &response, nullptr);

                if (cntl.Failed()) {
                    std::cerr << "Sync failed: " << cntl.ErrorText() << std::endl;
                    sleep(sync_interval_sec);
                    break;
                }

                leveldb::WriteBatch batch;
                for (const auto& item : response.items()) {
                    std::cout << "[Synced] " << item.key() << " => " << item.value() << std::endl;
                    next_message_id = item.message_id();
                    batch.Put(item.key(), item.value());
                }
                db->writeBatch(batch);

                has_more = response.has_more();
                if (!has_more) {
                    last_message_id = next_message_id;
                    std::cout << "One sync cycle complete. Last message_id = " << last_message_id << std::endl;
                }
            }

            sleep(sync_interval_sec);
        }
    }
}