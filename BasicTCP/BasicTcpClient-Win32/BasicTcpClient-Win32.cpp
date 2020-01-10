// BasicTcpClient-Win32.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
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

static const int BUFLEN = 512;
static const char* PORT = "11000";

SOCKET DoConnect(PADDRINFOA addr)
{
    SOCKET s = socket(addr->ai_family, addr->ai_socktype,
        addr->ai_protocol);
    if (s == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
    }
    else {
        // Connect to server.
        int iResult = connect(s, addr->ai_addr, (int)addr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(s);
            return INVALID_SOCKET;
        }
    }
    return s;
}

void PressEnter(const char* waitText) {
    std::cout << "Press ENTER to " << waitText;
    std::string dummy;
    std::getline(std::cin, dummy);
}

void RunSendLoop(SOCKET s) {
    std::string message;
    for (;;) {
        std::cout << ">";
        std::getline(std::cin, message);

        send(s, message.c_str(), message.length(), 0);
        
        if (message == "exit.") {
            break;
        }
    }
}

int main()
{
    WSADATA wsaData;

    const char* sendbuf = "this is a test";
    char recvbuf[BUFLEN];
    int iResult;
    int recvbuflen = BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ADDRINFOA hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    PADDRINFOA addr = NULL;
    iResult = getaddrinfo("localhost", PORT, &hints, &addr);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    PressEnter("connect ...");
    
    int cnt = 0;
    SOCKET s = DoConnect(addr);
    freeaddrinfo(addr);

    if (s != INVALID_SOCKET) {
        std::cout << "CONNECTED!!!!\n";

        RunSendLoop(s);

        closesocket(s);
    }
    
    WSACleanup();

    PressEnter("close");
    return 0;
}
