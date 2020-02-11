#pragma once

#include "Common.hpp"

#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class BasicServer {
private:
    sockaddr_in _serverAddress;
    int _listenerSocket;

public:
    BasicServer(const char* ipAddressStr, const uint16_t port)
        : _serverAddress(), _listenerSocket(-1)
    {
        _serverAddress.sin_addr.s_addr = inet_addr(ipAddressStr);
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
    }

    void CreateListenerSocket() {
        _listenerSocket = TRY("Socket creation",
            socket(AF_INET, SOCK_STREAM, 0)
            );
        TRY("Bind", bind(_listenerSocket, (sockaddr*)&_serverAddress, sizeof(_serverAddress)));
        TRY("Listen", listen(_listenerSocket, 10));
    }

    ~BasicServer() {
        if (_listenerSocket == -1) return;
        TRY("Close socket", close(_listenerSocket));
        _listenerSocket = -1;
    }

};
