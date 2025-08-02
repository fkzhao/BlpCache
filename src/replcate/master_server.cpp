//
// Created by fakzhao on 2025/8/2.
//

#include "master_server.h"
#include "core/db.h"
#include <butil/logging.h>

namespace blp {
    void MasterServer::SyncData(
            google::protobuf::RpcController* cntl_base,
            const SyncRequest* request,
            SyncResponse* response,
            google::protobuf::Closure* done) {

        brpc::Controller* cntl =
            static_cast<brpc::Controller*>(cntl_base);
        LOG(INFO) << "Received request[log_id=" << cntl->log_id()
                  << "] from " << cntl->remote_side()
                  << " to " << cntl->local_side()
                  << ": " << request->has_last_message_id()
                  << " (attached=" << cntl->request_attachment() << ")";

        brpc::ClosureGuard done_guard(done);
        auto db = LevelDBWrapper::getInstance();
        std::vector<DataItem> items;
        if (!request->has_last_message_id() || request->last_message_id().empty()) {
            db->getAllData(items);
            response->set_is_full_sync(true);
        } else {
            db->getIncrementalData(request->last_message_id(), items);
            response->set_is_full_sync(false);
        }

        for (auto& item : items) {
            DataItem* resp_item = response->add_items();
            resp_item->set_message_id(item.message_id());
            resp_item->set_key(item.key());
            resp_item->set_value(item.value());
        }

        response->set_has_more(items.size() >= 100);

    }
}