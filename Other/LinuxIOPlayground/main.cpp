#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <liburing.h>

#include "Common.hpp"

void DoVectoredIo() {
    const char msg1[] = "lol";
    const char msg2[] = "baz";

    iovec bufs[] = {
        {.iov_base = (void*)msg1, .iov_len = sizeof(msg1) - 1 },
        {.iov_base = (void*)msg2, .iov_len = sizeof(msg2) - 1 },
    };

    int fd = TRY(open("./lol.baz", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH));
    TRY(writev(fd, bufs, 2));

    TRYZ(close(fd));
}

const int RING_ENTRY_COUNT = 128;

void HelloUring() {
    io_uring ring;

    TRY(io_uring_queue_init(RING_ENTRY_COUNT, &ring, 0));

    int fd = TRY(open("./lol.baz", O_RDONLY));

    TRY(io_uring_register_files(&ring, &fd, 1));
    

    char msg1[32] = { };
    char msg2[32] = { };


    iovec bufs[] = {
        {.iov_base = (void*)msg1, .iov_len = sizeof(msg1) - 1 },
        {.iov_base = (void*)msg2, .iov_len = sizeof(msg2) - 1 },
    };

    TRY(io_uring_register_buffers(&ring, bufs, 2));

    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    io_uring_prep_rw(IORING_OP_READV, sqe, fd, bufs, 1, 0);
    sqe->user_data = 42;
    TRY(io_uring_submit(&ring));

    io_uring_cqe* cqe;
    TRY(io_uring_wait_cqe(&ring, &cqe));

    std::cout << "UserData: " << cqe->user_data << "Result: " << cqe->res << std::endl;
    std::cout << msg1 << std::endl;
    

    io_uring_cqe_seen(&ring, cqe);

    TRY(io_uring_unregister_buffers(&ring));
    TRY(io_uring_unregister_files(&ring));

    TRY(close(fd));
    io_uring_queue_exit(&ring);
}

int main()
{
    DoVectoredIo();
    HelloUring();

    PressEnter2("Close");
    return 0;
}