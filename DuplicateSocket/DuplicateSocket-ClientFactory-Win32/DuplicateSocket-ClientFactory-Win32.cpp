// BasicTcpClient-Win32.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

static const int BUFLEN = 512;
static const char* PORT = "11000";

SOCKET DoConnect(PADDRINFOA addr)
{
    /*SOCKET s = socket(addr->ai_family, addr->ai_socktype,
        addr->ai_protocol);*/

    SOCKET s = WSASocket(addr->ai_family, addr->ai_socktype, addr->ai_protocol, NULL, NULL, WSA_FLAG_OVERLAPPED);

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

HANDLE FindProcessByName(const std::wstring& name) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            std::wstring exeFile(entry.szExeFile);

            if (exeFile == name)
            {
                return OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            }
        }
    }

    CloseHandle(snapshot);

    throw std::runtime_error("process not found!");
}


void PressEnter(const char* waitText) {
    std::cout << "Press ENTER to " << waitText;
    std::string dummy;
    std::getline(std::cin, dummy);
}


class CompletionWorker {
    HANDLE _thread;
    SOCKET _socket;
    DWORD _threadId;
    HANDLE _completionPort;

    WSABUF _wsaBuf;
    char _buffer[512];
    OVERLAPPED _overlapped;

    bool _stop;

    void RunThreadWorker() {
        std::cout << "ThreadWorker runs !!!" << std::endl;

        OVERLAPPED* pOverlapped = NULL;
        while (!_stop) {
            DWORD received;
            if (GetQueuedCompletionStatus(_completionPort, &received, (PULONG_PTR)this, &pOverlapped, 100)) {
                std::cout << "Got some reply !!!" << std::endl;

                _buffer[received] = 0;
                std::string echo(_buffer);
                std::cout << "ECHO:" << echo << std::endl;
            }

        }

        std::cout << "ThreadWorker stopped !!!" << std::endl;
    }

    static DWORD WINAPI ThreadWorker(LPVOID lpParam) {
        CompletionWorker* self = (CompletionWorker*)lpParam;
        self->RunThreadWorker();
        return 0;
    }


public:
    CompletionWorker(SOCKET s) :
        _threadId(0),
        _socket(s),
        _thread(CreateThread(0, 0, ThreadWorker, this, 0, &_threadId)),
        _completionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)),
        _wsaBuf(),
        _buffer(),
        _overlapped(),
        _stop(false)
    {
        _wsaBuf.buf = _buffer;
        _wsaBuf.len = 512;

        if (_thread == nullptr) {
            throw std::runtime_error("CreateThread failed");
        }
        if (_thread == nullptr) {
            throw std::runtime_error("CreateIoCompletionPort failed");
        }

        HANDLE hTemp = CreateIoCompletionPort((HANDLE)s, _completionPort, (DWORD)this, 0);
        if (hTemp == nullptr) {
            throw std::runtime_error("Binding failed");
        }
    }

    void Stop() {
        _stop = true;
    }

    void ReceiveAsync() {
        DWORD flags = 0;
        WSARecv(_socket, &_wsaBuf, 1, NULL, &flags, &_overlapped, NULL);
    }

    ~CompletionWorker() {
        WaitForSingleObject(_thread, INFINITE);
        CloseHandle(_completionPort);
    }
};


void RunSendLoop(SOCKET s) {
    std::string message;

    CompletionWorker worker(s);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (;;) {
        std::cout << ">";
        std::getline(std::cin, message);

        send(s, message.c_str(), message.length(), 0);

        if (message == "exit.") {
            worker.Stop();
            break;
        }

        if (message[message.length() - 1] == '.') {
            worker.ReceiveAsync();
        }
    }
}

const char* PROTOCOL_INFO_FILE = "..\\_ProtocolInfo.bin";

int main()
{
    WSADATA wsaData;
    int iResult;

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

    HANDLE clientProcess = FindProcessByName(L"DuplicateSocket-Client-Win32.exe");
    DWORD clientProcessId = GetProcessId(clientProcess);

    int cnt = 0;
    SOCKET s = DoConnect(addr);
    freeaddrinfo(addr);
    
    if (s != INVALID_SOCKET) {
        std::cout << "CONNECTED!!!!\n";

        WSAPROTOCOL_INFOW protocolInfo = {};

        if (WSADuplicateSocket(s, clientProcessId, &protocolInfo) == 0) {
            std::ofstream fs(PROTOCOL_INFO_FILE, std::ofstream::binary);
            fs.write((char*)&protocolInfo, sizeof(WSAPROTOCOL_INFOW));
        }
        else {
            std::cout << "WSADuplicateSocket failed!\n";
        }

        PressEnter("close socket and exit");
        closesocket(s);
        WSACleanup();
    }

    
    return 0;
}
