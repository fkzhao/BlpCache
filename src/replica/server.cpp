//
// Created by fakzhao on 2025/8/6.
//

#include "server.h"
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

#include "common/socket_utils.h"
#include "common/ring_buffer.h"
#include "protocol.h"

namespace blp {
    constexpr size_t CHUNK_SIZE = 4096;
    auto &buffer = RingBuffer::getInstance();

    void send_full_sync(int client_fd) {
        uint64_t total_size = 100;
        std::cout << "[Master] Full sync done (" << total_size << " bytes)\n";
    }

    void handle_replica(int client_fd) {
        std::cout << "[Master] Replica connected: FD " << client_fd << "\n";
        if (!send_ack(client_fd, 0)) {
            std::cerr << "Replica disconnected or error.\n";
        }
        auto last_heartbeat_time = std::chrono::steady_clock::now();
        while (true) {
            Message header;
            if (!header.recvFromSocket(client_fd, header)) {
                std::cerr << "[Master] Failed to receive header, disconnecting...\n";
                break;
            }
            if (header.type == FULL_SYNC) {
                std::cout << "[Master] Sending FULL_SYNC to replica\n";
                send_full_sync(client_fd);
            } else if (header.type == INCR_SYNC) {
                std::cout << "[Master] Sending INCR_SYNC to replica\n";
                // Handle incremental sync logic here
            } else if (header.type == HEARTBEAT) {
                std::cout << "[Master] Received HEARTBEAT from replica\n";
                last_heartbeat_time = std::chrono::steady_clock::now();
            } else if (header.type == ACK) {
                std::cout << "[Master] Received ACK from replica\n";
            } else {
                std::cerr << "[Master] Unknown message type: " << int(header.type) << "\n";
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_time).count() > 10) {
                std::cerr << "[Replica] Heartbeat timeout, reconnecting...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        close(client_fd);
        std::cout << "[Master] Replica closed: FD " << client_fd << "\n";
    }

    void simulate_data_append(int client_fd) {
        while (true) {
            const auto item = buffer.pop();
            if (!item.has_value()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; // No data to send, wait for more
            }
            const auto& dataEntity = item.value();
            std::cout << "[Master] Simulating data append: sequence " << dataEntity.sequence()
                      << ", key " << dataEntity.key() << ", value " << dataEntity.value() << "\n";
            if (Message msg =  Message::fromDataEntity(INCR_SYNC, dataEntity); !msg.sendToSocket(client_fd)) {
                std::cerr << "[Master] Failed to send data to replica, disconnecting...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void ReplicationServer::startServer(const uint16_t port) const {
        int server_fd = SocketUtils::create_server_socket(port);
        std::cout << "[Master] Listening on port " << port << "...\n";


        while (running) {
            int client_fd = SocketUtils::accept_client(server_fd);
            std::thread t(handle_replica, client_fd);
            t.detach();
            std::thread data_thread(simulate_data_append, client_fd);
            data_thread.detach();
        }

        close(server_fd);
    }
}
