#pragma once

#include <liburing.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <string_view>

#include "BasicServer.hpp"

enum class Operation {
    POLL_LISTENER,
    ACCEPT,
    POLL_HANDLER,
    RECEIVE,
    SEND,

    POLL_ABORT
};

struct CompletionInfo {
    int fd;
    Operation operation;
    iovec iov;
    msghdr msg;
    char buffer[BasicServer::MAX_MESSAGE];
    
    std::string collector;

    CompletionInfo() :
        fd(-1),
        iov { .iov_base = buffer, .iov_len = BasicServer::MAX_MESSAGE },
        msg {
            .msg_iov = &iov,
            .msg_iovlen = 1,
        },
        collector()
    {
    }
};

class SubmissionInfo {
    const int _fd;
    io_uring_sqe* _sqe;
    CompletionInfo* _completion;
    
    void SetupCompletionData(Operation operation) {
        io_uring_sqe_set_data(_sqe, _completion);
        _completion->operation = operation;
    }

    msghdr* GetMsg() {
        msghdr& msg = _completion->msg;
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        return &msg;
    }

public:  
    SubmissionInfo(int fd, io_uring_sqe* sqe, CompletionInfo* completion) : 
        _fd(fd),
        _sqe(sqe),
        _completion(completion)
    {
    }

    void PrepareAccept() {
        io_uring_prep_accept(_sqe, _fd, NULL, NULL, SOCK_NONBLOCK);
        SetupCompletionData(Operation::ACCEPT);
    }

    void PreparePoll(Operation operation, short mask = POLLIN) {
        io_uring_prep_poll_add(_sqe, _fd, mask);
        SetupCompletionData(operation);
    }

    void PrepareReceive() {
        io_uring_prep_recvmsg(_sqe, _fd, GetMsg(), 0);
        SetupCompletionData(Operation::RECEIVE);
    }

    void PrepareSend() {
        io_uring_prep_sendmsg(_sqe, _fd, GetMsg(), 0);
        SetupCompletionData(Operation::SEND);
    }
};


class UringServer : public BasicServer
{
public:
    static const int RING_ENTRIES = 512;
    static const int INFO_TABLE_SIZE = RING_ENTRIES + 8;

private:
    io_uring_params _ringParams;
	io_uring _ring;
    int _abortEvt;

    CompletionInfo _completionData[INFO_TABLE_SIZE];

    static CompletionInfo* GetCompletionInfo(io_uring_cqe* cqe) {
        void* data = io_uring_cqe_get_data(cqe);
        if (data == nullptr) {
            throw std::runtime_error("User Data is null!");
        }
        return reinterpret_cast<CompletionInfo*>(data);
    }

    SubmissionInfo CreateSubmission(int fd) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        CompletionInfo* c = &_completionData[fd];
        c->fd = fd;
        return SubmissionInfo(fd, sqe, c);
    }

    bool ProcessCompletion(CompletionInfo* c, io_uring_cqe* cqe) {
        switch (c->operation) {
            case Operation::POLL_ABORT: {
                ASSERT(c->fd == _abortEvt);
                std::cout << "Abortion evt signaled, terminating" << std::endl;
                return false;
            }    
            case Operation::ACCEPT: {
                ASSERT(c->fd == _listenerSocket);
                int handlerSocket = cqe->res;
                if (handlerSocket < 0) {
                    throw std::runtime_error("Accept failed!");
                }
                std::cout << "accepted handler " << handlerSocket << std::endl;

                CreateSubmission(_listenerSocket).PrepareAccept();
                CreateSubmission(handlerSocket).PreparePoll(Operation::POLL_HANDLER);

                break;
            }
            case Operation::POLL_HANDLER: {
                if (cqe->res >= 0) {
                    // std::cout << "Got something on " << c->fd << std::endl;
                    CreateSubmission(c->fd).PrepareReceive();
                }
                else {
                    std::cout << -cqe->res << "," << std::flush;
                }

                break;
            }
            case Operation::RECEIVE: {
                int byteCount = cqe->res;
                if (byteCount >= 0) {
                    // std::cout << "Received " << cqe->res << "bytes on " << c->fd << std::endl;
                    std::string_view s(c->buffer, byteCount);
                    c->collector += s;
                    
                    if (s == "exit.") {
                        std::cout << "received exit signal, terminating " << std::endl;
                        return false;
                    }
                    else if (s[s.length() - 1] == '.') {
                        std::string& s = c->collector;
                        s.resize(s.length() - 1);

                        std::cout << "RECEIVED: " << s << std::endl;
                        
                        s += "!!!";
                        s.copy(c->buffer, s.length());
                        c->iov.iov_len = s.length();

                        CreateSubmission(c->fd).PrepareSend();
                        s.clear();
                    }
                    else {
                        CreateSubmission(c->fd).PreparePoll(Operation::POLL_HANDLER);
                    }
                }
                else {
                    std::cout << "Read failed:" << -cqe->res << std::endl;
                }

                break;
            }
            case Operation::SEND: {
                std::cout << "Data sent! " << cqe->res << std::endl;
                CreateSubmission(c->fd).PreparePoll(Operation::POLL_HANDLER);
                break;
            }
            default: {
                std::cout << "Unexpected operation" << std::endl;
                break;
            }
        }
        return true;
    }

public:

    UringServer(sa_family_t addressFamily, const char* ipAddressStr, const uint16_t port, uint32_t ipv6ScopeId = 0) : 
        BasicServer(addressFamily, ipAddressStr, port, ipv6ScopeId), 
        _ringParams(), 
        _ring(), 
        _completionData()
    {
    }

    void Initialize() override {
        TRYNE(io_uring_queue_init_params(RING_ENTRIES, &_ring, &_ringParams));
        
        BasicServer::Initialize();

        _abortEvt = TRY(eventfd(0, 0));
        CreateSubmission(_abortEvt).PreparePoll(Operation::POLL_ABORT);
        CreateSubmission(_listenerSocket).PrepareAccept();
    }

    ~UringServer() override {
        if (_ring.ring_fd > 0) {
            io_uring_queue_exit(&_ring);
        }

        BasicServer::~BasicServer();
    }

    void HandleRequests() override {
        io_uring_cqe* cqe;
        
        while (true) {
            
            TRYNE(io_uring_submit_and_wait(&_ring, 1)); // this is the only syscall
            TRYNE(io_uring_peek_cqe(&_ring, &cqe));

            CompletionInfo* c = GetCompletionInfo(cqe);

            if (!ProcessCompletion(c, cqe)) return;

            io_uring_cq_advance(&_ring, 1);
        }
    }

    void EndHandlingRequests() override {
        std::cout << "sending abort event" << std::endl;
        eventfd_write(_abortEvt, 1);

        std::cout << "joining thread" << std::endl;
        TRYZ(pthread_join(_handlerThread, NULL));
    }
};