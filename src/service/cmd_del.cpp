//
// Created by fakzhao on 2025/8/1.
//

#include "cmd_del.h"
#include "common/config.h"
namespace blp {
    brpc::RedisCommandHandlerResult DelCommandHandler::Run(
            brpc::RedisConnContext* ctx,
            const std::vector<butil::StringPiece>& args,
            brpc::RedisReply* output,
            bool /*flush_batched*/) {
        if (blp::config::model != "master") {
            output->FormatError("slaver can not run 'del' command");
            return brpc::REDIS_CMD_HANDLED;
        }
        const auto* session = dynamic_cast<AuthSession*>(ctx->get_session());
        if (session == nullptr) {
            output->FormatError("No auth session");
            return brpc::REDIS_CMD_HANDLED;
        }
        if (session->_password.empty()) {
            output->FormatError("No user name");
            return brpc::REDIS_CMD_HANDLED;
        }
        if (args.size() != 2ul) {
            output->FormatError("Expect 1 arg for 'del', actually %lu", args.size() - 1);
            return brpc::REDIS_CMD_HANDLED;
        }
        if (const std::string key(args[1].data(), args[1].size()); _rsimpl->Del(key)) {
            output->SetStatus("OK");
        } else {
            output->SetNullString();
        }
        return brpc::REDIS_CMD_HANDLED;
    }

} // namespace blp