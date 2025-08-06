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
#include "replcate/master_server.h"


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
    if (!blp::init_db()) {
        fprintf(stderr, "error init db. \n");
        return -1;
    }

    // config brpc server
    LOG(INFO) << "Starting Redis Service...";
    brpc::Server redis_server;
    brpc::ServerOptions redis_server_options;

    // init redis service
    const auto redis_service_impl = blp::init_redis_service();
    redis_server_options.redis_service = redis_service_impl;

    // start server
    if (redis_server.Start(6479, &redis_server_options) != 0) {
        LOG(ERROR) << "Fail to start server";
        return -1;
    }
    LOG(INFO) << "Redis Service started on port 6479";

    // init master server
    LOG(INFO) << "Starting Master Server...";
    brpc::Server master_server;
    blp::MasterServer master_server_impl;
    if (master_server.AddService(&master_server_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }
    brpc::ServerOptions master_options;
    master_options.idle_timeout_sec = 60;
    if (master_server.Start("127.0.0.1:6480", &master_options) != 0) {
        LOG(ERROR) << "Fail to start SyncServer";
        return -1;
    }
    LOG(INFO) << "Master Server started on port 6480";
    std::thread t1([&]{ redis_server.RunUntilAskedToQuit(); });
    std::thread t2([&]{ master_server.RunUntilAskedToQuit(); });
    t1.join();
    t2.join();
    return 0;
}