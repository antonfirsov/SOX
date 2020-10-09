#include <cstdio>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <iostream>

static void* connect_socket(void* arg)
{
    int s = (int)(long)arg;

    struct sockaddr_in6 address;
    address.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::ffff:1.1.1.1", &address.sin6_addr);
    address.sin6_port = htons(23);

    int rv = connect(s, (struct sockaddr*)&address, sizeof(address));
    assert(rv != 0);

    return NULL;
}

int main()
{
    // create AF_INET6 socket
    int s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    // on-going connect to 1.1.1.1:23
    pthread_t connect_thread;
    int rv = pthread_create(&connect_thread, NULL, connect_socket, (void*)(long)s);
    assert(rv == 0);
    sleep(1); // sleep to wait until connect is on-going.

    std::cout << "connect(AF_UNSPEC) ..." << std::endl;
    // connect to AF_UNSPEC
    struct sockaddr address;
    address.sa_family = AF_UNSPEC;
    rv = connect(s, &address, sizeof(address));

    if (rv < 0 && errno == EINVAL) {
        std::cout << "EINVAL! trying shutdown" << std::endl;

        rv = shutdown(s, SHUT_RDWR);
        if (rv < 0)
        {
            std::cout << "rv: " << rv << " errno: " << errno << std::endl;
        }
        else {
            std::cout << "SUCCESS!" << std::endl;
        }
    }

    std::cout << "pthread_join ..." << std::endl;

    // wait for thread
    rv = pthread_join(connect_thread, NULL);
    assert(rv == 0);

    std::cout << "DONE." << std::endl;

    return 0;
}