//
// Created by fakzhao on 2025/8/7.
//

#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace blp {
    class SocketUtils {
    public:
        static int create_server_socket(uint16_t port);
        static int accept_client(int server_fd);
        static int create_client_socket(const std::string& ip, uint16_t port);

        static bool send_all(int sockfd, const void* buffer, size_t length);
        static bool recv_all(int sockfd, void* buffer, size_t length);
    };
}