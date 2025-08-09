//
// Created by fakzhao on 2025/8/7.
//

#include "socket_utils.h"
#include <arpa/inet.h>
#ifdef __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
#include <fcntl.h>
#include <errno.h>


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

    bool SocketUtils::recv_all_with_event(int epoll_fd, int sockfd, void* buffer, size_t length, int timeout_ms) {
#ifdef __APPLE__
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        int kq = kqueue();
        if (kq == -1) {
            perror("kqueue");
            return false;
        }

        struct kevent change;
        EV_SET(&change, sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
        if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
            perror("kevent register");
            close(kq);
            return false;
        }

        size_t received = 0;
        char* ptr = static_cast<char*>(buffer);

        while (received < length) {
            ssize_t n = recv(sockfd, ptr + received, length - received, 0);
            if (n > 0) {
                received += n;
                continue;
            } else if (n == 0) {
                close(kq);
                return false;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    struct kevent event;
                    struct timespec ts;
                    ts.tv_sec = timeout_ms / 1000;
                    ts.tv_nsec = (timeout_ms % 1000) * 1000000;

                    int nev = kevent(kq, NULL, 0, &event, 1, &ts);
                    if (nev == 0) {
                        close(kq);
                        return false;
                    } else if (nev < 0) {
                        perror("kevent wait");
                        close(kq);
                        return false;
                    }

                    if (event.filter == EVFILT_READ) {
                        continue;
                    }
                }
                perror("recv");
                close(kq);
                return false;
            }
        }
        close(kq);
        return true;
#else
        int epoll_fd = epoll_create(0);
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sockfd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

        char buf[1024];
        size_t received = 0;
        char* ptr = static_cast<char*>(buffer);
        while (received < length) {
            ssize_t n = recv(sockfd, ptr + received, length - received, 0);

            if (n > 0) {
                received += n;
                continue;
            }
            else if (n == 0) {
                return false;
            }
            else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    struct epoll_event ev;
                    int nfds = epoll_wait(epoll_fd, &ev, 1, timeout_ms);
                    if (nfds == 0) {
                        return false;
                    }
                    if (nfds < 0) {
                        return false;
                    }
                    if (ev.data.fd != sockfd) {
                        continue;
                    }
                    continue;
                }
                return false;
            }
        }
        return true;
#endif
    }
}