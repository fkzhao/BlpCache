//
// Created by fakzhao on 2025/8/6.
//

#include "replicate.h"
#include "replication.pb.h"

namespace blp {

    class StreamClientReceiver final : public brpc::StreamInputHandler {
    public:
        int on_received_messages(brpc::StreamId id,
                                         butil::IOBuf *const messages[],
                                         size_t size) override {
            std::ostringstream os;
            for (size_t i = 0; i < size; ++i) {
                os << "msg[" << i << "]=" << *messages[i];
            }
            auto res = brpc::StreamWrite(id, *messages[0]);
            LOG(INFO) << "Received from Stream=" << id << ": " << os.str() << " and write back result: " << res;
            return 0;
        }
        void on_idle_timeout(brpc::StreamId id) override {
            LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
        }
        void on_closed(brpc::StreamId id) override {
            LOG(INFO) << "Stream=" << id << " is closed";
        }

        virtual void on_finished(brpc::StreamId id, int32_t finish_code) {
            LOG(INFO) << "Stream=" << id << " is finished, code " << finish_code;
        }
    };


    void StartReplication(const std::string& master_addr) {
        brpc::Channel channel;
        brpc::ChannelOptions options;
        options.protocol = "baidu_std";
        options.connection_type = "single";
        options.timeout_ms = 10000;
        // options.idle_timeout_ms = -1;

        if (channel.Init(master_addr.c_str(), &options) != 0) {
            std::cerr << "Failed to init channel to master" << std::endl;
            return;
        }

        ReplicationService_Stub stub(&channel);
        brpc::Controller cntl;

        brpc::StreamId stream_id;
        brpc::StreamOptions stream_options;
        StreamClientReceiver receiver;
        stream_options.handler = &receiver;
        if (brpc::StreamCreate(&stream_id, cntl, &stream_options) != 0) {
            std::cerr << "Failed to start Sync stream" << std::endl;
            return;
        }
        while (!brpc::IsAskedToQuit()) {
            sleep(1);
        }

        // std::ofstream aof_file("replica_data.aof", std::ios::binary | std::ios::trunc);
        //
        // SyncMessage msg;
        // while (receiver.Read(&msg)) {
        //     if (msg.type() == SyncMessage::FULL_DATA) {
        //         aof_file.write(msg.full_chunk().data(), msg.full_chunk().size());
        //         std::cout << "Received AOF chunk, size=" << msg.full_chunk().size() << std::endl;
        //         if (msg.is_last_chunk()) {
        //             std::cout << "Full AOF sync completed." << std::endl;
        //             // 可以在这里加载数据到内存或DB
        //         }
        //     } else if (msg.type() == SyncMessage::INCREMENT_DATA) {
        //         const auto& entry = msg.entry();
        //         std::cout << "Received increment data: seq=" << entry.sequence()
        //                   << ", key=" << entry.key()
        //                   << ", value=" << entry.value() << std::endl;
        //
        //         // TODO: 写入本地AOF或内存DB
        //     }
        // }
        // std::cout << "Sync stream closed." << std::endl;
    }

}