#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <iostream>
#include <string>
#include <chrono>
#include <thread>


void InitWinsock() {
    WSADATA wsaData;
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}


int main()
{
    InitWinsock();

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (s == INVALID_SOCKET) {
        std::cout << "socket creation failed" << std::endl;
        return 1;
    }

    std::cout << "Lol?!\n";

    return 0;
}
