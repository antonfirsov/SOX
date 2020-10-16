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

void AbortConnectTest()
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
}

static int client;
static sockaddr_in6 address;

void* AbortReceiveTest_Connect(void* arg)
{
    connect(client, (sockaddr*)&address, sizeof(address));
    return NULL;
}

void* AbortReceiveTest_Receive(void* arg) {
    char buffer[1];
    recv(client, buffer, 1, 0);
    return NULL;
}

void AbortReceiveTest()
{
    int listener = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    address = { };
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_loopback;
    address.sin6_port = htons(8887);

    bind(listener, (sockaddr*)&address, sizeof(address));
    listen(listener, 10);

    client = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    pthread_t connect_thread;
    int rv = pthread_create(&connect_thread, NULL, AbortReceiveTest_Connect, NULL);
    assert(rv == 0);

    int server = accept(listener, NULL, NULL);

    rv = pthread_join(connect_thread, NULL);
    assert(rv == 0);


    std::cout << "yeah." << std::endl;

    pthread_t recv_thread;
    rv = pthread_create(&recv_thread, NULL, AbortReceiveTest_Receive, NULL);
    assert(rv == 0);

    /*rv = shutdown(client, SHUT_RDWR);*/

    sockaddr address = { };
    address.sa_family = AF_UNSPEC;
    rv = connect(client, &address, sizeof(address));

    if (rv < 0)
    {
        std::cout << "connect(AF_UNSPEC) failed! rv: " << rv << " errno: " << errno << std::endl;
        std::cout << "trying shutdown .." << std::endl;

        rv = shutdown(client, SHUT_RDWR);

        if (rv < 0)
        {
            std::cout << "shutdown failed! rv: " << rv << " errno: " << errno << std::endl;
            return;
        }
        else 
        {
            std::cout << "shutdown succeeded!" << std::endl;
        }
    }
    else {
        std::cout << "connect(AF_UNSPEC) succeeded!" << std::endl;
    }

    rv = pthread_join(connect_thread, NULL);
    assert(rv == 0);
    std::cout << "YEAH????" << std::endl;

    char buffer[1];
    rv = recv(server, buffer, 1, 0);

    std::cout << "recv on server returned: " << rv << std::endl;
}

int main()
{
    AbortReceiveTest();

    return 0;
}