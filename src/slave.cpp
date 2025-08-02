//
// Created by fakzhao on 2025/8/3.
//
#include <butil/logging.h>
#include <brpc/channel.h>
#include "replcate/sync_service.pb.h"



int main(int argc, char* argv[]) {
    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    options.timeout_ms = 5000;
    if (channel.Init("127.0.0.1:6480", &options) != 0) {
        std::cerr << "Fail to init channel" << std::endl;
        return -1;
    }

    blp::SyncService_Stub stub(&channel);
    std::string last_message_id = "";
    bool full_sync_done = false;

    while (true) {
        std::string next_message_id = last_message_id;
        bool has_more = true;

        while (has_more) {
            blp::SyncRequest request;
            request.set_slave_id("1");
            if (!next_message_id.empty()) {
                request.set_last_message_id(next_message_id);
            }

            blp::SyncResponse response;
            brpc::Controller cntl;
            stub.SyncData(&cntl, &request, &response, nullptr);

            if (cntl.Failed()) {
                std::cerr << "Sync failed: " << cntl.ErrorText() << std::endl;
                break;
            }

            for (const auto& item : response.items()) {
                std::cout << "[Synced] " << item.key() << " => " << item.value() << std::endl;
                next_message_id = item.message_id();
            }

            has_more = response.has_more();
            if (!has_more) {
                last_message_id = next_message_id;
                std::cout << "One sync cycle complete. Last message_id = " << last_message_id << std::endl;
            }
        }
        sleep(1);
    }
    return 0;
}