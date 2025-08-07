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

namespace blp {
    constexpr size_t CHUNK_SIZE = 4096;

    std::atomic<bool> s_running{true};
    std::vector<int> replicas;
    std::mutex replica_mutex;


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
        while (s_running) {
            MessageHeader header{};
            if (!SocketUtils::recv_all(client_fd, &header, sizeof(header))) {
                std::cerr << "[Master] Failed to receive header, disconnecting...\n";
                break;
            }
            header.from_network_order();
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

    void simulate_data_append() {
        int counter = 1;
        while (s_running) {
            std::string data = "SET key" + std::to_string(counter) + " value" + std::to_string(counter) + "\n";
            std::cout << "[Master] Appended: " << data;
            counter++;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    void start_replication_server(uint16_t port) {
        int server_fd = SocketUtils::create_server_socket(port);
        std::cout << "[Master] Listening on port " << port << "...\n";

        std::thread data_thread(simulate_data_append);

        while (s_running) {
            int client_fd = SocketUtils::accept_client(server_fd);
            std::thread t(handle_replica, client_fd);
            t.detach();
        }

        data_thread.join();
        close(server_fd);
    }
}
