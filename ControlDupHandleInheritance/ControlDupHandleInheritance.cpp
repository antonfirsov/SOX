// ControlDupHandleInheritance.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <tlhelp32.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "Kernel32.lib")

#include <iostream>

void PrintHandleInformation(SOCKET s, const char* name) {
    DWORD info;
    if (GetHandleInformation((HANDLE)s, &info))
    {
        bool inheritable = (info & HANDLE_FLAG_INHERIT) == HANDLE_FLAG_INHERIT;
        bool protectFromClose = (info & HANDLE_FLAG_PROTECT_FROM_CLOSE) == HANDLE_FLAG_PROTECT_FROM_CLOSE;
        std::cout << name << " inheritable: " << inheritable << " protectFromClose: " << protectFromClose << std::endl;
    }
}

int main()
{
    WSADATA wsaData; 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    // Make sure original socket is not inheritable:
    DWORD e_x_p_e_c_t_e_d = WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT;
    SOCKET original = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, e_x_p_e_c_t_e_d);
    if (original == INVALID_SOCKET) return 1;

    SetHandleInformation((HANDLE)original, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
    PrintHandleInformation(original, "Original");

    WSAPROTOCOL_INFOW protocolInfo = {};
    DWORD processId = GetCurrentProcessId();

    if (WSADuplicateSocket(original, processId, &protocolInfo) != 0) return 1;
    closesocket(original);

    // Try to enforce the same behavior on clone handle:
    SOCKET clone = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &protocolInfo, 0, e_x_p_e_c_t_e_d);
    if (clone == INVALID_SOCKET) return 1;

    PrintHandleInformation(clone, "Clone");
    closesocket(clone);

    WSACleanup();

    return 0;
}

/* OUTPUT

Original inheritable: 0
Clone inheritable: 1

*/