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

        brpc::StreamId stream;
        brpc::StreamOptions stream_options;
        StreamClientReceiver receiver;
        stream_options.handler = &receiver;
        if (brpc::StreamCreate(&stream, cntl, &stream_options) != 0) {
            std::cerr << "Failed to start Sync stream" << std::endl;
            return;
        }

        while (!brpc::IsAskedToQuit()) {
            butil::IOBuf msg1;
            Message message;
            message.set_type(Message_MsgType_FULL_DATA);
            std::string serialized_data;
            if (!message.SerializeToString(&serialized_data)) {
                LOG(ERROR) << "Failed to serialize message";
                continue;
            }
            msg1.append(serialized_data);
            CHECK_EQ(0, brpc::StreamWrite(stream, msg1));
            butil::IOBuf msg2;
            msg2.append("0123456789");
            CHECK_EQ(0, brpc::StreamWrite(stream, msg2));
            sleep(1);
        }
    }

}