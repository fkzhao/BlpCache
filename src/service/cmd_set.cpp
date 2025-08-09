//
// Created by fakzhao on 2025/8/1.
//

#include "cmd_set.h"
#include "common/config.h"
#include "replica/protocol.h"

namespace blp {
    brpc::RedisCommandHandlerResult SetCommandHandler::Run(brpc::RedisConnContext *ctx,
                                                           const std::vector<butil::StringPiece> &args,
                                                           brpc::RedisReply *output,
                                                           bool /*flush_batched*/) {
        if (blp::config::model != "master") {
            output->FormatError("slaver can not run 'set' command");
            return brpc::REDIS_CMD_HANDLED;
        }
        const auto *session = dynamic_cast<AuthSession *>(ctx->get_session());
        if (session == nullptr) {
            output->FormatError("No auth session");
            return brpc::REDIS_CMD_HANDLED;
        }
        if (session->_password.empty()) {
            output->FormatError("No user name");
            return brpc::REDIS_CMD_HANDLED;
        }
        if (args.size() != 3ul) {
            output->FormatError("Expect 2 args for 'set', actually %lu", args.size() - 1);
            return brpc::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        const std::string value(args[2].data(), args[2].size());
        if (const bool rs = _rsimpl->Set(key, value); !rs) {
            output->FormatError("Failed to set key '%s'", key.c_str());
            return brpc::REDIS_CMD_HANDLED;
        }
        output->SetStatus("OK");
        return brpc::REDIS_CMD_HANDLED;
    }
}
