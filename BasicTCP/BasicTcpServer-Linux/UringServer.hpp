#pragma once

#include <liburing.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <string_view>
#include <atomic>

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

    CompletionInfo() : fd(-1)
    {
    }
};

struct SendReceiveData {
    iovec iov;
    msghdr msg;
    char buffer[BasicServer::MAX_MESSAGE];
    std::string collector;

    SendReceiveData() :
        iov{ .iov_base = buffer, .iov_len = BasicServer::MAX_MESSAGE },
        msg{
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
    SendReceiveData* _srData;

    void SetupCompletionData(Operation operation) {
        io_uring_sqe_set_data(_sqe, _completion);
        _completion->operation = operation;
    }

    msghdr* GetMsg() {
        msghdr& msg = _srData->msg;
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        return &msg;
    }

public:  
    SubmissionInfo(int fd, io_uring_sqe* sqe, CompletionInfo* completion, SendReceiveData* srData) : 
        _fd(fd),
        _sqe(sqe),
        _completion(completion),
        _srData(srData)
    {
    }

    SubmissionInfo& PrepareAccept() {
        io_uring_prep_accept(_sqe, _fd, NULL, NULL, SOCK_NONBLOCK);
        SetupCompletionData(Operation::ACCEPT);
        return *this;
    }

    SubmissionInfo& PreparePoll(Operation operation, short mask = POLLIN) {
        io_uring_prep_poll_add(_sqe, _fd, mask);
        SetupCompletionData(operation);
        return *this;
    }

    SubmissionInfo& PrepareReceive() {
        io_uring_prep_recvmsg(_sqe, _fd, GetMsg(), 0);
        SetupCompletionData(Operation::RECEIVE);
        return *this;
    }

    SubmissionInfo& PrepareSend() {
        io_uring_prep_sendmsg(_sqe, _fd, GetMsg(), 0);
        SetupCompletionData(Operation::SEND);
        return *this;
    }

    SubmissionInfo& AppendFlags(unsigned char flags) {
        _sqe->flags |= flags;
        return *this;
    }
};


class UringServer : public BasicServer
{
public:
    static const int RING_ENTRIES = 512;

private:
    io_uring_params _ringParams;
	io_uring _ring;
    int _abortEvt;

    std::atomic<size_t> _completionDataCounter;
    CompletionInfo _completionData[RING_ENTRIES];
    SendReceiveData _sendReceiveData[RING_ENTRIES + 8];

    static CompletionInfo* GetCompletionInfo(io_uring_cqe* cqe) {
        void* data = io_uring_cqe_get_data(cqe);
        if (data == nullptr) {
            throw std::runtime_error("User Data is null!");
        }
        return reinterpret_cast<CompletionInfo*>(data);
    }

    SubmissionInfo CreateSubmission(int fd) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        size_t idx = _completionDataCounter++ % RING_ENTRIES;

        CompletionInfo* c = &_completionData[idx];
        SendReceiveData* srData = &_sendReceiveData[fd];
        c->fd = fd;
        return SubmissionInfo(fd, sqe, c, srData);
    }

    void SubmitLinkedPollRead(int socket) {
        CreateSubmission(socket).PreparePoll(Operation::POLL_HANDLER).AppendFlags(IOSQE_IO_LINK);
        CreateSubmission(socket).PrepareReceive();
    }

    bool ProcessCompletion(CompletionInfo* c, int result) {
        switch (c->operation) {
            case Operation::POLL_ABORT: {
                ASSERT(c->fd == _abortEvt);
                std::cout << "Abortion evt signaled, terminating" << std::endl;
                return false;
            }    
            case Operation::ACCEPT: {
                ASSERT(c->fd == _listenerSocket);
                if (result < 0) {
                    throw std::runtime_error("Accept failed!");
                }
                std::cout << "accepted handler " << result << std::endl;

                CreateSubmission(_listenerSocket).PrepareAccept();
                SubmitLinkedPollRead(result);

                break;
            }
            case Operation::POLL_HANDLER: {
                if (result >= 0) {
                    std::cout << "Got something on " << c->fd << std::endl;
                }
                else {
                    std::cout << "Poll failed: " << -result << std::endl;
                }

                break;
            }
            case Operation::RECEIVE: {
                if (result >= 0) {
                    SendReceiveData* sr = &_sendReceiveData[c->fd];

                    std::string_view s(sr->buffer, result);
                    sr->collector += s;
                    
                    if (s == "exit.") {
                        std::cout << "received exit signal, terminating " << std::endl;
                        return false;
                    }
                    else if (s[s.length() - 1] == '.') {
                        std::string& s = sr->collector;
                        s.resize(s.length() - 1);

                        std::cout << "RECEIVED: " << s << std::endl;
                        
                        s += "!!!";
                        s.copy(sr->buffer, s.length());
                        sr->iov.iov_len = s.length();

                        CreateSubmission(c->fd).PrepareSend();
                        s.clear();
                    }
                    else {
                        SubmitLinkedPollRead(c->fd);
                    }
                }
                else {
                    std::cout << "Read failed:" << -result << std::endl;
                }

                break;
            }
            case Operation::SEND: {
                SubmitLinkedPollRead(c->fd);
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
        _completionDataCounter(0),
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
            int result = cqe->res;
            io_uring_cq_advance(&_ring, 1);

            if (!ProcessCompletion(c, result)) return;
        }
    }

    void EndHandlingRequests() override {
        std::cout << "sending abort event" << std::endl;
        eventfd_write(_abortEvt, 1);

        std::cout << "joining thread" << std::endl;
        TRYZ(pthread_join(_handlerThread, NULL));
    }
};