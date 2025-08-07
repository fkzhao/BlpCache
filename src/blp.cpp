#include <brpc/server.h>
#include <brpc/redis.h>
#include <butil/crc32c.h>
#include <butil/strings/string_split.h>
#include <gflags/gflags.h>
#include <thread>
#include <string>
#include <leveldb/db.h>

#include <butil/time.h>

#include "common/config.h"
#include "service/service.h"
#include "common/sequence_number.h"
#include "common/aof.h"
#include "replica/server.h"
#include "replica/replicate.h"

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    // load configuration
    const std::string conf_file = std::string(getenv("BLP_HOME")) + "/conf/blp.conf";
    if (!blp::config::init(conf_file)) {
        fprintf(stderr, "error read config file. \n");
        return -1;
    }

    std::cout <<"BLP Cache Start On: " << blp::config::port << std::endl;

    // init db
    // if (!blp::init_db()) {
    //     fprintf(stderr, "error init db. \n");
    //     return -1;
    // }

    // config brpc server
    // LOG(INFO) << "Starting Redis Service...";
    // brpc::Server redis_server;
    // brpc::ServerOptions redis_server_options;
    //
    // // init redis service
    // const auto redis_service_impl = blp::init_redis_service();
    // redis_server_options.redis_service = redis_service_impl;
    //
    // // start server
    // if (redis_server.Start(blp::config::port, &redis_server_options) != 0) {
    //     LOG(ERROR) << "Fail to start server";
    //     return -1;
    // }
    // LOG(INFO) << "Redis Service started on port " << blp::config::port;
    // redis_server.RunUntilAskedToQuit();
    // std::thread t1([&]{ redis_server.RunUntilAskedToQuit(); });
    if (blp::config::model == "master") {
        std::thread t2([&]{ blp::start_replication_server(blp::config::replica_port); });
        t2.join();
    } else if (blp::config::model == "replicate") {
        LOG(INFO) << "Starting Replication Service...";
        std::string host = "127.0.0.1";
        std::thread t2([&]{ blp::start_replica(host.data(), blp::config::replica_port); });
        t2.join();
    }  else {
        LOG(ERROR) << "Unknown model: " << blp::config::model;
        return -1;
    }
    return 0;
}