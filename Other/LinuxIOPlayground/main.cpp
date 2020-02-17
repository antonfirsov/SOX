#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Common.hpp"

void DoVectoredIo() {
    const char msg1[] = "lol";
    const char msg2[] = "baz";

    iovec bufs[] = {
        {.iov_base = (void*)msg1, .iov_len = sizeof(msg1) - 1 },
        {.iov_base = (void*)msg2, .iov_len = sizeof(msg2) - 1 },
    };

    int fd = TRY(open("./bitchy.bitch", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH));
    TRY(writev(fd, bufs, 2));

    TRYZ(close(fd));
}

int main()

{
    DoVectoredIo();

    std::cout << getpagesize() << std::endl;

    PressEnter2("Close");
    return 0;
}