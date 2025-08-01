//
// Created by fakzhao on 2025/8/1.
//

#include "cmd_auth.h"


namespace blp {
    brpc::RedisCommandHandlerResult AuthCommandHandler::Run(
            brpc::RedisConnContext* ctx,
            const std::vector<butil::StringPiece>& args,
            brpc::RedisReply* output,
            bool flush_batched) {
        if (args.size() != 2ul) {
            output->FormatError("Expect 1 arg for 'auth', actually %lu", args.size() - 1);
            return brpc::REDIS_CMD_HANDLED;
        }
        const std::string password(args[1].data(), args[1].size());
        if (RedisServiceImpl::Auth(password)) {
            output->SetStatus("OK");
            const auto auth_session = new AuthSession(password);
            ctx->reset_session(auth_session);
        } else {
            output->FormatError("Invalid password for database");
        }
        return brpc::REDIS_CMD_HANDLED;
    }

}