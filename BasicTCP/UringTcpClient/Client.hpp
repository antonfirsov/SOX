#pragma once

#include <iostream> 
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>

#include "Common.hpp"


class Client {
    static const int RING_SIZE = 128;

    sockaddr_in _serverAddress;
    int _socket;
    io_uring _ring;
    iovec _iovec;
    msghdr _msg;

    char _buffer[256];

    static void CheckCqe(const char* opName, io_uring_cqe* cqe) {
        if (cqe->res < 0) {
            std::stringstream ss;
            ss << opName << " failed with: " << cqe->res;
            throw std::runtime_error(ss.str());
        }   
    }

    void PrepareMsg() {
        _msg.msg_name = &_serverAddress;
        _msg.msg_namelen = sizeof(_serverAddress);
    }

public:
    Client(const char* ipAddressStr, const uint16_t port) : 
        _serverAddress(),
        _socket(-1),
        _ring(),
        _iovec{ .iov_base = _buffer, .iov_len = sizeof(_buffer) - 1 },
        _msg {
            .msg_name = &_serverAddress,
            .msg_namelen = sizeof(_serverAddress),
            .msg_iov = &_iovec,
            .msg_iovlen = 1,
        },
        _buffer()
    {
        _serverAddress.sin_addr.s_addr = inet_addr(ipAddressStr);
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
    }

    void Initialize() {
        _socket = TRY(socket(AF_INET, SOCK_STREAM, 0));

        TRY(io_uring_queue_init(RING_SIZE, &_ring, 0));
        TRY(io_uring_register_buffers(&_ring, &_iovec, 1));
        TRY(io_uring_register_files(&_ring, &_socket, 1));
    }

    void Connect() {
        TRY(connect(_socket, (sockaddr*)&_serverAddress, sizeof(_serverAddress)));
    }

    ~Client() {
        if (_ring.ring_fd > 0) {
            io_uring_unregister_files(&_ring);
            io_uring_unregister_buffers(&_ring);
            io_uring_queue_exit(&_ring);
        }

        if (_socket != -1) close(_socket);
    }

    void DoSmalltalk() {
        io_uring_cqe* cqe;

        while (true) {
            std::cout << ">";
            std::string s;
            std::cin >> s;

            s.copy(_buffer, s.length());
            _iovec.iov_len = s.length();

            io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
            io_uring_prep_sendmsg(sqe, _socket, &_msg, 0);

            TRY(io_uring_submit(&_ring));
            TRY(io_uring_wait_cqe(&_ring, &cqe));
            
            CheckCqe("sendmsg", cqe);
            std::cout << "Sent: " << cqe->res << std::endl;
            io_uring_cqe_seen(&_ring, cqe);

            if (s[s.length() - 1] == '.') {
                sqe = io_uring_get_sqe(&_ring);
                _iovec.iov_len = sizeof(_buffer) - 1;
                io_uring_prep_recvmsg(sqe, _socket, &_msg, 0);
                TRY(io_uring_submit(&_ring));
                TRY(io_uring_wait_cqe(&_ring, &cqe));

                CheckCqe("receivemsg", cqe);
                int bytesReceived = cqe->res;
                io_uring_cqe_seen(&_ring, cqe);

                if (bytesReceived > 0) {
                    std::string s(_buffer, bytesReceived - 1);
                    std::cout << "echo: " << s << std::endl;
                }
            }
            if (s == "exit.") {
                break;
            }
        }
    }
};