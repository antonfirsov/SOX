#pragma once

#include "Common.hpp"

#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

void* _RunServerThread(void* arg);

class BasicServer {
protected:
    sa_family_t _addressFamily;
    sockaddr _sockaddr;

    int _listenerSocket;
    const char* _ipStr;
    const uint16_t _port;
    pthread_t _handlerThread;

    sockaddr_in& Sockaddr4() {
        return reinterpret_cast<sockaddr_in&>(_sockaddr);
    }

    sockaddr_in6& Sockaddr6() {
        return reinterpret_cast<sockaddr_in6&>(_sockaddr);
    }

    const socklen_t AddrLen() const {
        return _addressFamily == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    }

public:

    const int MAX_PENDING = 10;

    BasicServer(sa_family_t addressFamily, const char* ipAddressStr, const uint16_t port, uint32_t ipv6ScopeId = 0) : 
        _addressFamily(addressFamily), 
        _sockaddr(), 
        _listenerSocket(-1), 
        _ipStr(ipAddressStr),
        _port(port),
        _handlerThread()
    {
        if (addressFamily == AF_INET) {
            sockaddr_in& a = Sockaddr4();
            
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = inet_addr(ipAddressStr);
            a.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
        }
        else if (addressFamily == AF_INET6) {
            sockaddr_in6& a = Sockaddr6();
            a.sin6_family = AF_INET6;
            TRY(inet_pton(AF_INET6, ipAddressStr, &a.sin6_addr));
            a.sin6_port = htons(port);
            //a.sin6_scope_id = ipv6ScopeId;
        }
        else {
            throw new std::runtime_error("Invalid address family!");
        }   
    }

    virtual void Initialize() {
        _listenerSocket = TRY(socket(_addressFamily, SOCK_STREAM, IPPROTO_TCP));
        TRY(bind(_listenerSocket, &_sockaddr, AddrLen()));
        TRY(listen(_listenerSocket, MAX_PENDING));

        std::cout << "*** SERVER listening at " << _ipStr << " " << _port << std::endl;
    }

    virtual ~BasicServer() {
        if (_listenerSocket == -1) return;
        close(_listenerSocket);
    }

    virtual void HandleRequests() {

        char buffer[256];

        while (true) {
            std::cout << "Waiting for connection ... " << std::flush;

            int handlerSocket = TRY(accept(_listenerSocket, NULL, NULL));
            std::cout << " connected." << std::endl;

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
