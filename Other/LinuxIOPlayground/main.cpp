#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <liburing.h>

void Assert(bool condition, const char* operationStr) {
    if (!condition) {
        std::cout << "Failed: " << operationStr << " | Errno: " << errno << std::endl;
        exit(1);
    }
}

void Try(int result, const char* operationStr) {
    if (result < 0) {
        std::cout << "Failed: " << operationStr << " | Result: " << result << " Errno: " << errno << std::endl;
        exit(1);
    }
}

#define ASSERT( conditionExpr ) Assert(conditionExpr, #conditionExpr )
#define TRY( operation ) Try( operation, #operation )

const int SERVER_PORT = 11000;

int main()
{
    sockaddr_in listenerAddr {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = htons(SERVER_PORT),
    };
    sockaddr* listenerAddrPtr = (sockaddr*)&listenerAddr;

    int listener = socket(AF_INET, SOCK_STREAM, 0); ASSERT(listener != -1);
    
    TRY(bind(listener, (sockaddr*)&listenerAddr, sizeof(listenerAddr)));
    TRY(listen(listener, 1));

    sockaddr_in clientAddr {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = 0,
    };
    int client = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); ASSERT(client != -1);
    TRY(bind(client, (sockaddr*)&clientAddr, sizeof(clientAddr)));
    TRY(fcntl(client, F_SETFL, O_NONBLOCK));

    ASSERT(connect(client, listenerAddrPtr, sizeof(listenerAddr)) == -1); ASSERT(errno == EINPROGRESS);

    io_uring serverRing;
    TRY(io_uring_queue_init(64, &serverRing, 0));
    io_uring_cqe* cqe;

    //int server = accept4(listener, NULL, NULL, SOCK_NONBLOCK); ASSERT(server != -1);

    io_uring_sqe* acceptSqe = io_uring_get_sqe(&serverRing);
    io_uring_prep_accept(acceptSqe, listener, NULL, NULL, SOCK_NONBLOCK);
    acceptSqe->user_data = 666;

    TRY(io_uring_submit(&serverRing));

    TRY(io_uring_wait_cqe(&serverRing, &cqe));
    ASSERT(cqe->user_data == 666);
    int server = cqe->res; ASSERT(server > 0);
    io_uring_cqe_seen(&serverRing, cqe);

    char sndBuf[4];
    ASSERT(send(client, sndBuf, 4, 0) == 4); ASSERT(errno == EINPROGRESS);

    io_uring_sqe* pollSqe = io_uring_get_sqe(&serverRing);
    io_uring_prep_poll_add(pollSqe, server, POLLIN);
    pollSqe->flags |= IOSQE_IO_LINK;
    pollSqe->user_data = 1;

    
    char recvBuf[512];
    iovec recvIovec{
        .iov_base = recvBuf,
        .iov_len = sizeof(recvBuf)
    };

    io_uring_sqe* readSqe = io_uring_get_sqe(&serverRing);
    msghdr msg{
        .msg_iov = &recvIovec,
        .msg_iovlen = 1
    };
    io_uring_prep_recvmsg(readSqe, server, &msg, 0);
    readSqe->user_data = 2;


    TRY(io_uring_submit(&serverRing));
    
    TRY(io_uring_wait_cqe(&serverRing, &cqe));
    std::cout << cqe->user_data << " " << cqe->res <<  std::endl;
    ASSERT(cqe->user_data == 1 && cqe->res >= 0);
    io_uring_cqe_seen(&serverRing, cqe);

    TRY(io_uring_wait_cqe(&serverRing, &cqe));
    //std::cout << cqe->user_data << " " << cqe->res << std::endl;
    ASSERT(cqe->user_data == 2 && cqe->res == 4);
    io_uring_cqe_seen(&serverRing, cqe);

    TRY(close(client));
    TRY(close(server));
    TRY(close(listener));
    return 0;
}