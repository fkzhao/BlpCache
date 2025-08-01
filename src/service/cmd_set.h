//
// Created by fakzhao on 2025/8/1.
//

#pragma once

#include "cmd_base.h"

namespace blp {
    class SetCommandHandler final : public CommandHandler {
    public:
        explicit SetCommandHandler(RedisServiceImpl *redis_service_impl): CommandHandler(redis_service_impl) {
        }

        brpc::RedisCommandHandlerResult Run(
            brpc::RedisConnContext *ctx,
            const std::vector<butil::StringPiece> &args,
            brpc::RedisReply *output, bool) override;
    };
}
