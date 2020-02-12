#pragma once

#include "Common.hpp"

#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

void* _RunServerThread(void* arg);

class BasicServer {
private:
    sockaddr_in _serverAddress;
    int _listenerSocket;
    const char* _ipStr;
    pthread_t _handlerThread;

    const sockaddr* ServerSockaddr() const {
        return reinterpret_cast<const sockaddr*>(&_serverAddress);
    }

public:
    BasicServer(sa_family_t addressFamily, const char* ipAddressStr, const uint16_t port)
        : _serverAddress(), _listenerSocket(-1), _ipStr(ipAddressStr), _handlerThread()
    {
        _serverAddress.sin_addr.s_addr = inet_addr(ipAddressStr);
        _serverAddress.sin_family = addressFamily;
        _serverAddress.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
    }

    void Initialize() {
        _listenerSocket = TRY(socket(AF_INET, SOCK_STREAM, 0));
        TRY(bind(_listenerSocket, ServerSockaddr(), sizeof(_serverAddress)));
        TRY(listen(_listenerSocket, 10));

        std::cout << "*** SERVER listening at " << _ipStr << " " << _serverAddress.sin_port << std::endl;
    }

    ~BasicServer() {
        if (_listenerSocket == -1) return;
        close(_listenerSocket);
    }

    void HandleRequests() {

        char buffer[256];

        while (true) {
            std::cout << "Waiting for connection ... ";

            int handlerSocket = TRY(accept(_listenerSocket, NULL, NULL));

            std::string message;

            while (true) {
                long count = TRY(recv(handlerSocket, buffer, 256, 0));
                auto part = std::string(buffer, static_cast<size_t>(count));
                message += part;
                if (part[part.length() - 1] == '.') {

                    message = message.substr(0, message.length() - 1);

                    std::cout << "RECEIVED: " << message << std::endl;

                    if (part == "exit.") break;

                    message += "!!!";
                    TRY(send(handlerSocket, message.c_str(), message.length(), 0));
                    message.clear();
                }
            }
        }
    }

    void BeginHandlingRequests() {
        TRYZ(pthread_create(&_handlerThread, NULL, _RunServerThread, this));
    }

    void EndHandlingRequests() {
        TRYZ(pthread_cancel(_handlerThread));
        TRYZ(pthread_join(_handlerThread, NULL));
    }
};

void* _RunServerThread(void* arg) {

    BasicServer* server = static_cast<BasicServer*>(arg);
    server->HandleRequests();

    return NULL;
}
