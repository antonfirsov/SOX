#pragma once

#include <iostream> 
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void PressEnter2(const char* opName) {
    std::cout << "Press Enter to " << opName << '!' << std::endl;
    char dummy[128];
    std::cin.getline(dummy, 128);
}

template<typename F>
auto TryStuff(const char* opName, F&& lambda) -> decltype(lambda()) {
    auto fd = lambda();
    if (fd < 0) throw std::runtime_error(opName + std::string(" failed!"));
    return fd;
}


#define TRY( opName, operation ) TryStuff(opName, [&]() { return operation; })

class Client {
private:
    sockaddr_in _serverAddress;
    int _socket;

public:
    Client(const char* ipAddressStr, const uint16_t port)
        : _serverAddress(), _socket(-1)
    {
        _serverAddress.sin_addr.s_addr = inet_addr(ipAddressStr);
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
    }

    void CreateSocket() {
        _socket = TRY("Socket creation",
            socket(AF_INET, SOCK_STREAM, 0)
            );
    }

    void Connect() {
        TRY("Connect",
            connect(_socket, (sockaddr*)&_serverAddress, sizeof(_serverAddress))
            );
    }

    ~Client() {
        if (_socket == -1) return;
        TRY("Close socket", close(_socket));
        _socket = -1;
    }

    void DoSmalltalk() {
        while (true) {
            std::cout << ">";
            std::string s;
            std::cin >> s;

            TRY("send",
                send(_socket, s.c_str(), s.length(), 0)
                );

            if (s[s.length() - 1] == '.') {
                char rcvBuf[256];
                auto bytesReceived = TRY("recv", recv(_socket, rcvBuf, 256, 0));
                if (bytesReceived > 0) {
                    std::string s(rcvBuf, bytesReceived - 1);
                    std::cout << "echo: " << s << std::endl;
                }
            }
            if (s == "exit.") {
                break;
            }
        }
    }
};