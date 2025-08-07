//
// Created by fakzhao on 2025/8/7.
//

#include "socket_utils.h"
#include <arpa/inet.h>

namespace blp {
    int SocketUtils::create_server_socket(uint16_t port) {
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw std::runtime_error("socket() failed");

        const int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");

        if (listen(fd, 5) < 0)
            throw std::runtime_error("listen() failed");

        return fd;
    }

    int SocketUtils::accept_client(int server_fd) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        return accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &len);
    }

    int SocketUtils::create_client_socket(const std::string& ip, uint16_t port) {
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw std::runtime_error("socket() failed");

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("connect() failed");

        return fd;
    }

    bool SocketUtils::send_all(int sockfd, const void* buffer, size_t length) {
        size_t sent = 0;
        const char* ptr = static_cast<const char *>(buffer);
        while (sent < length) {
            ssize_t n = send(sockfd, ptr + sent, length - sent, 0);
            if (n <= 0) return false;
            sent += n;
        }
        return true;
    }

    bool SocketUtils::recv_all(int sockfd, void* buffer, size_t length) {
        size_t received = 0;
        const auto ptr = static_cast<char *>(buffer);
        while (received < length) {
            ssize_t n = recv(sockfd, ptr + received, length - received, 0);
            if (n <= 0) return false;
            received += n;
        }
        return true;
    }
}