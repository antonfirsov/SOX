#pragma once

#include <liburing.h>
#include <sys/poll.h>
#include <sys/eventfd.h>


#include "BasicServer.hpp"

enum class Operation {
    POLL_LISTENER,
    ACCEPT,
    READ,
    WRITE,
    ABORT
};

struct CompletionInfo {
    int fd;
    Operation operation;
    iovec iov;
    char buffer[BasicServer::MAX_MESSAGE];

    CompletionInfo() :
        fd(-1),
        iov { .iov_base = buffer, .iov_len = BasicServer::MAX_MESSAGE }
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
        /*if (data == nullptr) {
            throw std::runtime_error("User Data is null!");
        }*/
        return reinterpret_cast<CompletionInfo*>(data);
    }

    SubmissionInfo CreateSubmission(int fd) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        CompletionInfo* c = &_completionData[fd];
        c->fd = fd;
        return SubmissionInfo(fd, sqe, c);
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
        CreateSubmission(_abortEvt).PreparePoll(Operation::ABORT);
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

            if (c == nullptr) {
                std::cout << "user_data is fucking null" << std::endl;
                io_uring_cq_advance(&_ring, 1);
                continue;
            }

            if (c->operation == Operation::ABORT) {
                std::cout << "Abortion evt signaled, terminating" << std::endl;
                return;
            }
            else if (c->fd == _listenerSocket) {
                if (c->operation == Operation::POLL_LISTENER) {
                    std::cout << "accept polled" << std::endl;

                    CreateSubmission(_listenerSocket).PrepareAccept();
                }
                else if (c->operation == Operation::ACCEPT) {
                   int handlerSocket = cqe->res;
                   if (handlerSocket < 0) {
                       throw std::runtime_error("Accept failed!");
                   }
                   std::cout << "accept received!" << std::endl;


                   CreateSubmission(_listenerSocket).PrepareAccept();
                   //CreateSubmission(handlerSocket).PrepareReceive();
                }
                else {
                    std::cout << "Unexpected operation on listener socket! " << std::endl;
                }
            }
            else {
                if (c->operation == Operation::READ) {
                    std::cout << "Received some data on " << c->fd << std::endl;
                    
                }
                else {
                    std::cout << "Unexpected operation on handler socket " << c->fd << std::endl;
                }
            }

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