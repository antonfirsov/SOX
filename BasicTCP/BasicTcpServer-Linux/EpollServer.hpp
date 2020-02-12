#pragma once

#include <unordered_map>
#include "BasicServer.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>


class EpollServer : public BasicServer
{
    static const int BUFFER_SIZE = 512;
    static const int MAX_EPOLL_EVENTS = 64;

    int _epoll;
    epoll_event _events[MAX_EPOLL_EVENTS];

    std::unordered_map<int, std::string> _messageBuilders;

    int _abortEvt;

    static void AppendEpollEvents(std::basic_ostream<char>& ss, uint32_t e) {
        APPEND_FLAG(ss, e, EPOLLIN);
        APPEND_FLAG(ss, e, EPOLLPRI);
        APPEND_FLAG(ss, e, EPOLLOUT);
        APPEND_FLAG(ss, e, EPOLLRDNORM);
        APPEND_FLAG(ss, e, EPOLLRDBAND);
        APPEND_FLAG(ss, e, EPOLLWRNORM);
        APPEND_FLAG(ss, e, EPOLLWRBAND);
        APPEND_FLAG(ss, e, EPOLLMSG);
        APPEND_FLAG(ss, e, EPOLLERR);
        APPEND_FLAG(ss, e, EPOLLHUP);
        APPEND_FLAG(ss, e, EPOLLRDHUP);
        APPEND_FLAG(ss, e, EPOLLEXCLUSIVE);
        APPEND_FLAG(ss, e, EPOLLWAKEUP);
        APPEND_FLAG(ss, e, EPOLLONESHOT);
        APPEND_FLAG(ss, e, EPOLLET);
    }

    static bool Check(const epoll_event& e, const EPOLL_EVENTS check) {
        return (e.events & check) == check;
    }

    void CloseHandlerSocket(int handlerSocket) {
        epoll_event evt = { };
        evt.data.fd = handlerSocket;
        TRYZ(epoll_ctl(_epoll, EPOLL_CTL_DEL, handlerSocket, &evt));
        TRYZ(close(handlerSocket));
        _messageBuilders.erase(handlerSocket);
    }

public:
    EpollServer(sa_family_t addressFamily, const char* ipAddressStr, const uint16_t port, uint32_t ipv6ScopeId = 0)
        : BasicServer(addressFamily, ipAddressStr, port, ipv6ScopeId), _epoll(-1), _events(), _messageBuilders(), _abortEvt(-1)
    {
    }

    void Initialize() override {
        BasicServer::Initialize();

        _epoll = TRY(epoll_create1(0));

        epoll_event evt = { };
        evt.data.fd = _listenerSocket;
        evt.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        TRYZ(epoll_ctl(_epoll, EPOLL_CTL_ADD, _listenerSocket, &evt));

        _abortEvt = eventfd(0, 0);
        evt.data.fd = _abortEvt;
        evt.events = EPOLLIN | EPOLLET;
        TRYZ(epoll_ctl(_epoll, EPOLL_CTL_ADD, _abortEvt, &evt));
    }

    ~EpollServer() override {
        if (_epoll != -1) close(_epoll);
        if (_abortEvt != -1) close(_abortEvt);

        BasicServer::~BasicServer();

        for (auto kv : _messageBuilders) {
            if (close(kv.first) == -1) std::cout << "failed to close [" << kv.first << "]" << std::endl;
        }
    }

    void HandleRequests() override {

        char buffer[BUFFER_SIZE];

        std::cout << "Listening ... " << std::endl;

        while (true) {
            int eventNo = TRY(epoll_wait(_epoll, _events, MAX_EPOLL_EVENTS, -1));

            for (int i = 0; i < eventNo; i++)
            {
                epoll_event& e = _events[i];
                if (e.data.fd == _listenerSocket) {

                    if (e.events == EPOLLIN) {
                        int handlerSocket = TRY(accept4(_listenerSocket, NULL, NULL, SOCK_NONBLOCK));
                        epoll_event evt = { };
                        evt.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        evt.data.fd = handlerSocket;

                        TRYZ(epoll_ctl(_epoll, EPOLL_CTL_ADD, handlerSocket, &evt));
                        std::cout << "Connection accepted! ->[" << handlerSocket << "]" << std::endl;
                        _messageBuilders[handlerSocket] = std::string();
                    }
                    else {
                        std::cout << "Unexpected events on listener socket: ";
                        AppendEpollEvents(std::cout, e.events);
                        std::cout << std::endl;
                    }
                }
                else if (e.data.fd == _abortEvt) {
                    std::cout << "Listener trhread detected abort event, terminating" << std::endl;
                    return;
                }
                else {
                    int handlerSocket = e.data.fd;

                    if (e.events == EPOLLIN) {
                        long count = TRY(recv(handlerSocket, buffer, BUFFER_SIZE, 0));

                        std::string& message = _messageBuilders[handlerSocket];
                        auto part = std::string(buffer, static_cast<size_t>(count));
                        message += part;
                        if (part[part.length() - 1] == '.') {
                            message = message.substr(0, message.length() - 1);

                            std::cout << "[" << handlerSocket << "] RECEIVED: " << message << std::endl;

                            if (part == "exit.") {
                                std::cout << "closing socket [" << handlerSocket << "]";
                                CloseHandlerSocket(handlerSocket);
                            }
                            else {
                                message += "!!!";

                                TRY(send(handlerSocket, message.c_str(), message.length(), 0));
                                message.clear();
                            }
                        }
                    }
                    else {
                        std::cout << "Unexpected events on handler [" << handlerSocket << "]: ";
                        AppendEpollEvents(std::cout, e.events);
                        std::cout << " closing" << std::endl;
                        CloseHandlerSocket(handlerSocket);
                    }
                }
            }
        }
    }


    void EndHandlingRequests() override {
        std::cout << "sending abort event" << std::endl;
        eventfd_write(_abortEvt, 1);

        std::cout << "joining thread" << std::endl;
        TRYZ(pthread_join(_handlerThread, NULL));
    }
};

