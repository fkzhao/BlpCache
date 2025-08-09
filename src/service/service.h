//
// Created by fakzhao on 2025/8/1.
//
#pragma once
#include <brpc/server.h>

#include "service/redis_service.h"
namespace blp {
    RedisServiceImpl* init_redis_service(LevelDBWrapper *db);
}