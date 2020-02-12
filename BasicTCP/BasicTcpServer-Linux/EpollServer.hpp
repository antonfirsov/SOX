#pragma once

#include <unordered_map>
#include "BasicServer.hpp"

#include <sys/epoll.h>

class EpollServer : public BasicServer
{
    static const int BUFFER_SIZE = 512;
    static const int MAX_EPOLL_EVENTS = 64;

    int _epoll;
    epoll_event _events[MAX_EPOLL_EVENTS];

    std::unordered_map<int, std::string> _messageBuilders;

public:
    EpollServer(sa_family_t addressFamily, const char* ipAddressStr, const uint16_t port, uint32_t ipv6ScopeId = 0)
        : BasicServer(addressFamily, ipAddressStr, port, ipv6ScopeId), _epoll(-1), _events(), _messageBuilders()
    {
    }

    void Initialize() override {
        BasicServer::Initialize();

        _epoll = TRY(epoll_create1(0));

        epoll_event evt = { };
        evt.data.fd = _listenerSocket;
        evt.events = EPOLLIN/* | EPOLLET*/;
        TRYZ(epoll_ctl(_epoll, EPOLL_CTL_ADD, _listenerSocket, &evt));
    }

    ~EpollServer() override {
        if (_epoll != -1) close(_epoll);

        BasicServer::~BasicServer();
    }

    void HandleRequests() override {

        char buffer[BUFFER_SIZE];

        epoll_event evt = { };

        std::cout << "Listening ... " << std::endl;

        while (true) {
            int eventNo = TRY(epoll_wait(_epoll, _events, MAX_EPOLL_EVENTS, -1));

            for (int i = 0; i < eventNo; i++)
            {
                epoll_event& e = _events[i];
                if (e.data.fd == _listenerSocket) {
                    int handlerSocket = TRY(accept4(_listenerSocket, NULL, NULL, SOCK_NONBLOCK));
                    evt.events = EPOLLIN | EPOLLET;
                    evt.data.fd = handlerSocket;

                    TRYZ(epoll_ctl(_epoll, EPOLL_CTL_ADD, handlerSocket, &evt));
                    std::cout << "Connection accepted! ->[" << handlerSocket << "]" << std::endl;
                    _messageBuilders[handlerSocket] = std::string();
                }
                else if (e.events == EPOLLIN) {
                    int handlerSocket = e.data.fd;
                    
                    long count = TRY(recv(handlerSocket, buffer, BUFFER_SIZE, 0));

                    std::string& message = _messageBuilders[handlerSocket];
                    auto part = std::string(buffer, static_cast<size_t>(count));
                    message += part;
                    if (part[part.length() - 1] == '.') {
                        message = message.substr(0, message.length() - 1);

                        std::cout << "[" << handlerSocket << "] RECEIVED: " << message << std::endl;

                        if (part == "exit.") {
                            std::cout << "closing socket [" << handlerSocket << "]";
                            TRYZ(close(handlerSocket));
                            _messageBuilders.erase(handlerSocket);
                            break;
                        }
                        message += "!!!";

                        TRY(send(handlerSocket, message.c_str(), message.length(), 0));
                        message.clear();
                    }
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

