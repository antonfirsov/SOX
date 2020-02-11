#pragma once

#include "Common.hpp"

#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <pthread.h>

void* _RunServerThread(void* arg);

class EpollServer {
    static const int BUFFER_SIZE = 512;
    static const int MAX_EPOLL_EVENTS = 64;

    sockaddr_in _serverAddress;
    const char* _ipStr;

    int _listenerSocket;
    
    pthread_t _handlerThread;

    int _epoll;
    epoll_event _events[MAX_EPOLL_EVENTS];

    const sockaddr* ServerSockaddr() const {
        return reinterpret_cast<const sockaddr*>(&_serverAddress);
    }

    

public:
    EpollServer(const char* ipAddressStr, const uint16_t port)
        : _serverAddress(), _listenerSocket(-1), _epoll(-1), _ipStr(ipAddressStr), _handlerThread()
    {
        _serverAddress.sin_addr.s_addr = inet_addr(ipAddressStr);
        _serverAddress.sin_family = AF_INET;
        _serverAddress.sin_port = htons(port); // little (x86-x64) -> big (tcpip)
    }

    void Initialize() {
        _listenerSocket = TRY("Socket creation",
            socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)
            );
        TRY("Bind", bind(_listenerSocket, ServerSockaddr(), sizeof(_serverAddress)));
        TRY("Listen", listen(_listenerSocket, 10));

        _epoll = TRY("epoll_create", epoll_create1(0));

        epoll_event evt = { };
        evt.data.fd = _listenerSocket;
        evt.events = EPOLLIN/* | EPOLLET*/;
        TRYZ("epoll_ctl", epoll_ctl(_epoll, EPOLL_CTL_ADD, _listenerSocket, &evt));

        std::cout << "*** SERVER listening at " << _ipStr << " " << _serverAddress.sin_port << std::endl;
    }

    ~EpollServer() {
        if (_listenerSocket == -1) return;
        TRY("Close socket", close(_listenerSocket));

        if (_epoll == -1) return;
        close(_epoll);
    }

    void HandleRequests() {

        char buffer[BUFFER_SIZE];

        epoll_event evt = { };

        while (true) {
            std::cout << "Waiting for connection ... ";

            //int handlerSocket = TRY("Accept", accept(_listenerSocket, NULL, NULL));

            int eventNo = TRY("epoll_wait", epoll_wait(_epoll, _events, MAX_EPOLL_EVENTS, -1));

            for (int i = 0; i < eventNo; i++)
            {
                epoll_event& e = _events[i];
                if (e.data.fd == _listenerSocket) {
                    int handlerSocket = TRY("accept4", accept4(_listenerSocket, NULL, NULL, SOCK_NONBLOCK));
                    evt.events = EPOLLIN | EPOLLET;
                    evt.data.fd = handlerSocket;

                    TRYZ("epoll_ctl (handler)", epoll_ctl(_epoll, EPOLL_CTL_ADD, handlerSocket, &evt));
                    std::cout << "Connection accepted!" << std::endl;
                }
            }

            /*std::string message;

            while (true) {
                long count = TRY("recv", recv(handlerSocket, buffer, BUFFER_SIZE, 0));

                auto part = std::string(buffer, static_cast<size_t>(count));
                message += part;
                if (part[part.length() - 1] == '.') {

                    message = message.substr(0, message.length() - 1);

                    std::cout << "RECEIVED: " << message << std::endl;

                    if (part == "exit.") break;

                    message += "!!!";
                    TRY("Send echo", send(handlerSocket, message.c_str(), message.length(), 0));
                    message.clear();
                }
            }*/
        }
    }

    void BeginHandlingRequests() {
        TRYZ("Thread creation", pthread_create(&_handlerThread, NULL, _RunServerThread, this));
    }

    void EndHandlingRequests() {
        TRYZ("Cancel thread", pthread_cancel(_handlerThread));
        TRYZ("Join thread", pthread_join(_handlerThread, NULL));
    }
};

void* _RunServerThread(void* arg) {

    EpollServer* server = static_cast<EpollServer*>(arg);
    server->HandleRequests();

    return NULL;
}
