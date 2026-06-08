#pragma once

#include <string>
#include <chrono>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

struct SocketError : std::runtime_error {
    explicit SocketError(const std::string& msg)
        : std::runtime_error(msg + ": " + std::strerror(errno)) {}
};

class Socket {
    int fd_;
public:
    Socket() : fd_(-1) {}
    explicit Socket(int domain, int type = SOCK_STREAM, int protocol = 0)
        : fd_(::socket(domain, type, protocol))
    {
        if (fd_ < 0) throw SocketError("socket");
    }

    ~Socket() { close(); }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) { close(); fd_ = other.fd_; other.fd_ = -1; }
        return *this;
    }

    int fd() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }

    void close() {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }

    void setReuseAddr(bool enable = true) {
        int opt = enable;
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw SocketError("setsockopt SO_REUSEADDR");
    }

    void setTimeout(std::chrono::milliseconds timeout) {
        struct timeval tv;
        tv.tv_sec  = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        if (setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            throw SocketError("setsockopt SO_RCVTIMEO");
        if (setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
            throw SocketError("setsockopt SO_SNDTIMEO");
    }

    void bind(const struct sockaddr_in& addr) {
        if (::bind(fd_, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw SocketError("bind");
    }

    void listen(int backlog = 5) {
        if (::listen(fd_, backlog) < 0)
            throw SocketError("listen");
    }

    Socket accept(struct sockaddr_in* peer = nullptr) const {
        socklen_t len = sizeof(struct sockaddr_in);
        struct sockaddr_in addr;
        int client = ::accept(fd_, (struct sockaddr*)&addr, peer ? &len : nullptr);
        if (client < 0) throw SocketError("accept");
        if (peer) *peer = addr;
        return Socket(FromFdTag{}, client);
    }

    void connect(const struct sockaddr_in& addr) {
        if (::connect(fd_, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw SocketError("connect");
    }

private:
    struct FromFdTag {};
    Socket(FromFdTag, int fd) : fd_(fd) {}
    friend Socket makeServerSocket(int port);
};

inline Socket makeServerSocket(int port) {
    Socket sock(AF_INET, SOCK_STREAM, 0);
    sock.setReuseAddr(true);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    sock.bind(addr);
    sock.listen();
    return sock;
}

inline struct sockaddr_in makeAddr(const std::string& ip, int port) {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
        throw SocketError("inet_pton (invalid IP)");
    return addr;
}

inline bool sendAll(int fd, const char* data, size_t size) {
    while (size > 0) {
        ssize_t n = ::send(fd, data, size, 0);
        if (n <= 0) return false;
        data += n;
        size -= n;
    }
    return true;
}

inline bool recvAll(int fd, char* buffer, size_t size) {
    while (size > 0) {
        ssize_t n = ::recv(fd, buffer, size, 0);
        if (n <= 0) return false;
        buffer += n;
        size -= n;
    }
    return true;
}

inline bool recvSome(int fd, char* buffer, size_t maxSize, int& received) {
    ssize_t n = ::recv(fd, buffer, maxSize, 0);
    if (n <= 0) return false;
    received = static_cast<int>(n);
    return true;
}
