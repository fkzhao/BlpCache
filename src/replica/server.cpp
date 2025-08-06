//
// Created by fakzhao on 2025/8/6.
//

#include "server.h"
#include <brpc/controller.h>
#include <brpc/stream.h>
#include <iostream>
#include <chrono>
#include <thread>

namespace blp {

    class StreamClientReceiver final : public brpc::StreamInputHandler {
    public:
        int on_received_messages(brpc::StreamId id,
                                 butil::IOBuf *const messages[],
                                 size_t size) override {
            std::ostringstream os;
            for (size_t i = 0; i < size; ++i) {
                os << "msg[" << i << "]=" << *messages[i];
            }
            auto res = brpc::StreamWrite(id, *messages[0]);
            LOG(INFO) << "Received from Stream=" << id << ": " << os.str() << " and write back result: " << res;
            return 0;
        }
        void on_idle_timeout(brpc::StreamId id) override {
            LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
        }
        void on_closed(brpc::StreamId id) override {
            LOG(INFO) << "Stream=" << id << " is closed";
        }

        virtual void on_finished(brpc::StreamId id, int32_t finish_code) {
            LOG(INFO) << "Stream=" << id << " is finished, code " << finish_code;
        }
    };

    ReplicationServiceImpl::ReplicationServiceImpl(const std::string &aof_file_path)
        : aof_file_path_(aof_file_path), stop_(false) {
    }

    ReplicationServiceImpl::~ReplicationServiceImpl() {
        stop_ = true;
    }

    void ReplicationServiceImpl::Sync(google::protobuf::RpcController *cntl_base,
                                      google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

    //     brpc::StreamId stream_id = cntl->stream_id();
    //     brpc::StreamSender sender(stream_id);
    //     brpc::StreamReceiver receiver(stream_id); {
    //         std::lock_guard<std::mutex> lock(clients_mutex_);
    //         clients_.push_back(ClientStream{sender, stream_id});
    //     }
    //
    //     std::cout << "New Replica connected, stream_id=" << stream_id << std::endl;
    //
    //     // 1. 先推送全量AOF文件流
    //     std::ifstream aof_file(aof_file_path_, std::ios::binary);
    //     if (!aof_file) {
    //         std::cerr << "Failed to open AOF file: " << aof_file_path_ << std::endl;
    //         return;
    //     }
    //
    //     const size_t chunk_size = 64 * 1024;
    //     char buffer[chunk_size];
    //     while (aof_file) {
    //         aof_file.read(buffer, chunk_size);
    //         size_t read_bytes = aof_file.gcount();
    //         if (read_bytes == 0) break;
    //
    //         replication::SyncMessage chunk_msg;
    //         chunk_msg.set_type(replication::SyncMessage::FULL_DATA);
    //         chunk_msg.set_full_chunk(std::string(buffer, read_bytes));
    //         chunk_msg.set_is_last_chunk(false);
    //
    //         if (!sender.Write(&chunk_msg)) {
    //             std::cerr << "Failed to send AOF chunk to replica stream " << stream_id << std::endl;
    //             break;
    //         }
    //     }
    //
    //     // 发送最后一个标记，表示AOF流传输结束
    //     replication::SyncMessage last_chunk;
    //     last_chunk.set_type(replication::SyncMessage::FULL_DATA);
    //     last_chunk.set_is_last_chunk(true);
    //
    //     sender.Write(&last_chunk);
    //
    //     // 2. 持续监听Replica发来的消息（心跳或控制消息，可忽略）
    //     replication::SyncMessage recv_msg;
    //     while (receiver.Read(&recv_msg)) {
    //         // 这里可以解析Replica发来的消息，做心跳或其他逻辑
    //     }
    //
    //     // 3. 连接断开，移除客户端
    //     std::cout << "Replica disconnected, stream_id=" << stream_id << std::endl; {
    //         std::lock_guard<std::mutex> lock(clients_mutex_);
    //         clients_.erase(std::remove_if(clients_.begin(), clients_.end(),
    //                                       [stream_id](const ClientStream &c) { return c.stream_id == stream_id; }),
    //                        clients_.end());
    //     }
    // }
    //
    // void ReplicationServiceImpl::BroadcastIncrement(const replication::DataEntry &entry) {
    //     std::lock_guard<std::mutex> lock(clients_mutex_);
    //     for (auto it = clients_.begin(); it != clients_.end();) {
    //         replication::SyncMessage msg;
    //         msg.set_type(replication::SyncMessage::INCREMENT_DATA);
    //         *msg.mutable_entry() = entry;
    //
    //         if (!it->sender.Write(&msg)) {
    //             std::cerr << "Failed to send increment data to stream " << it->stream_id << std::endl;
    //             it = clients_.erase(it);
    //         } else {
    //             ++it;
    //         }
    //     }
    }

    void ReplicationServiceImpl::PushIncrementalData(const DataEntry &entry) {
        BroadcastIncrement(entry);
    }

    void ReplicationServiceImpl::BroadcastIncrement(const DataEntry &entry) {}
}
