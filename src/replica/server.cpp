//
// Created by fakzhao on 2025/8/6.
//

#include "server.h"
#include <brpc/controller.h>
#include <brpc/stream.h>
#include <chrono>
#include <thread>

namespace blp {

    void ReplicationServiceImpl::Sync(google::protobuf::RpcController* controller,
                      const Message* request,
                      Message* response,
                      google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);

        auto* cntl =
            dynamic_cast<brpc::Controller*>(controller);
        brpc::StreamOptions stream_options;
        stream_options.handler = &_receiver;
        if (brpc::StreamAccept(&_sd, *cntl, &stream_options) != 0) {
            cntl->SetFailed("Fail to accept stream");
            return;
        }
        response->set_type(Message_MsgType_ACK);

    }
}
