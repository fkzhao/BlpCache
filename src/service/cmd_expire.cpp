//
// Created by fakzhao on 2025/8/9.
//

#include "cmd_expire.h"

namespace blp {
    brpc::RedisCommandHandlerResult ExpireCommandHandler::Run(
        brpc::RedisConnContext *ctx,
        const std::vector<butil::StringPiece> &args,
        brpc::RedisReply *output,
        bool /*flush_batched*/) {
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
            output->FormatError("Expect 2 arg for 'expire', actually %lu", args.size() - 1);
            return brpc::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        const std::string value(args[2].data(), args[2].size());

        try {
            int64_t seconds = std::stoll(std::string(args[2].data(), args[2].size()));
            if (_rsimpl->Expire(key, seconds)) {
                output->SetInteger(1);
            } else {
                output->SetInteger(0);
            }
        } catch (const std::exception&) {
            output->FormatError("Invalid expire time");
        }
        return brpc::REDIS_CMD_HANDLED;
    }
}