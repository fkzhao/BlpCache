//
// Created by fakzhao on 2025/8/6.
//

#pragma once

#include <brpc/stream.h>


#include "replica/replication.pb.h"


namespace blp {

    class StreamReceiver : public brpc::StreamInputHandler {
    public:
        virtual int on_received_messages(brpc::StreamId id,
                                         butil::IOBuf *const messages[],
                                         size_t size) {
            std::ostringstream os;
            for (size_t i = 0; i < size; ++i) {
                os << "msg[" << i << "]=" << *messages[i];
            }
            LOG(INFO) << "Received from Stream=" << id << ": " << os.str();
            return 0;
        }
        virtual void on_idle_timeout(brpc::StreamId id) {
            LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
        }
        virtual void on_closed(brpc::StreamId id) {
            LOG(INFO) << "Stream=" << id << " is closed";
        }

    };

    class ReplicationServiceImpl final : public ReplicationService {
    public:
        ReplicationServiceImpl() : _sd(brpc::INVALID_STREAM_ID) {}

        ~ReplicationServiceImpl() override {
            brpc::StreamClose(_sd);
        };

        void Sync(google::protobuf::RpcController* controller,
                      const Message* request,
                      Message* response,
                      google::protobuf::Closure* done);

    private:
        StreamReceiver _receiver;
        brpc::StreamId _sd;
    };
}