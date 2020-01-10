// DuplicateSocket-Client-Win32.cpp : This file contains the 'main' function. Program execution begins and ends there.
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


void PressEnter(const char* waitText) {
    std::cout << "Press ENTER to " << waitText;
    std::string dummy;
    std::getline(std::cin, dummy);
}

const char* PROTOCOL_INFO_FILE = "..\\_ProtocolInfo.bin";

WSAPROTOCOL_INFOW ReadProtocolInfo() {
    std::ifstream f(PROTOCOL_INFO_FILE, std::ifstream::binary);
    WSAPROTOCOL_INFOW protocolInfo;
    f.read((char*)&protocolInfo, sizeof(WSAPROTOCOL_INFOW));
    DeleteFileA(PROTOCOL_INFO_FILE);
    return protocolInfo;
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

    //CompletionWorker worker(s);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (;;) {
        std::cout << ">";
        std::getline(std::cin, message);

        send(s, message.c_str(), message.length(), 0);

        if (message == "exit.") {
            //worker.Stop();
            break;
        }

        if (message[message.length() - 1] == '.') {
            //worker.ReceiveAsync();

            char buffer[512];

            int received = recv(s, buffer, 512, 0);
            if (received > 0 && received < 512) {
                buffer[received] = 0;
                std::string echo(buffer);
                std::cout << "ECHO:" << echo << std::endl;
            }
        }
    }
}

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

    DeleteFileA(PROTOCOL_INFO_FILE);
    PressEnter("Read _ProtocolInfo.bin");

    WSAPROTOCOL_INFOW protocolInfo = ReadProtocolInfo();

    SOCKET s = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &protocolInfo, 0, 0);

    if (s != INVALID_SOCKET) {

        RunSendLoop(s);

        closesocket(s);
    }
    else {
        std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
    }

    PressEnter("die");
}
