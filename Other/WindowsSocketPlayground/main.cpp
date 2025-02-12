#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#include <iostream>
#include <string>
#include <thread>
#define SERVER_PORT 5000
#define BUFFER_SIZE 1024

void startServer();
void startClient();

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    std::thread serverThread(startServer);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Give server time to start
    std::thread clientThread(startClient);

    serverThread.join();
    clientThread.join();

    WSACleanup();

    std::cin.get();
    return 0;
}

void ConnectAsyncBlocking(SOCKET socket, const sockaddr* addr, int addrlen) {
    // Create an event object for synchronization
    HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (event == nullptr) {
        std::cerr << "CreateEvent failed." << std::endl;
        return;
    }

    // Create an OVERLAPPED structure and associate the event with it
    OVERLAPPED overlapped = {};
    overlapped.hEvent = event;

    

    // Initiate an asynchronous connection
    if (WSAConnect(socket, addr, addrlen, nullptr, nullptr, nullptr, &overlapped) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "WSAConnect failed." << WSAGetLastError() << std::endl;
            CloseHandle(event);
            throw std::runtime_error("WSAConnect failed");
        }
    }

    // Wait for the connection to complete
    DWORD bytesTransferred;
    if (WSAGetOverlappedResult(socket, &overlapped, &bytesTransferred, TRUE, nullptr) == FALSE) {
        std::cerr << "WSAGetOverlappedResult failed." << WSAGetLastError() << std::endl;
        throw std::runtime_error("WSAGetOverlappedResult failed");
    }
    else {
        std::cout << "Connection completed successfully." << WSAGetLastError() << std::endl;
    }

    // Clean up
    CloseHandle(event);
}

void disconnect(SOCKET socket, DWORD dwFlags) {
    GUID guidDisconnectEx = WSAID_DISCONNECTEX;
    LPFN_DISCONNECTEX lpfnDisconnectEx = nullptr;
    DWORD bytesReturned = 0;

    // Get the function pointer for DisconnectEx
    auto e = WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx, sizeof(guidDisconnectEx), &lpfnDisconnectEx, sizeof(lpfnDisconnectEx), &bytesReturned, nullptr, nullptr);
    if (e != 0) {
        std::cerr << "WSAIoctl failed to get DisconnectEx function pointer: " << e << " " << WSAGetLastError() << std::endl;
        return;
    }

    // Use DisconnectEx to disconnect the socket
    if (!lpfnDisconnectEx(socket, nullptr, dwFlags, 0)) {
        std::cerr << "DisconnectEx failed: " << WSAGetLastError() << std::endl;
    }
}

void startServer() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket." << std::endl;
        return;
    }

    const char* response = "Hello from server!";
    char buffer[BUFFER_SIZE];

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return;
    }

    std::cout << "Server is listening on port " << SERVER_PORT << std::endl;

    SOCKET handlerSocket1 = accept(serverSocket, nullptr, nullptr);
    if (handlerSocket1 == INVALID_SOCKET) {
        std::cerr << "Accept failed." << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return;
    }

    int bytesReceived = recv(handlerSocket1, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        std::cout << "Received message from client: " << std::string(buffer, 0, bytesReceived) << std::endl;
    }

    send(handlerSocket1, response, strlen(response), 0);
    disconnect(handlerSocket1, 0);
    closesocket(handlerSocket1);

    SOCKET handlerSocket2 = accept(serverSocket, nullptr, nullptr);
    if (handlerSocket2 == INVALID_SOCKET) {
        std::cerr << "Accept failed." << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return;
    }

    bytesReceived = recv(handlerSocket2, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        std::cout << "Received message from client: " << std::string(buffer, 0, bytesReceived) << std::endl;
    }
    send(handlerSocket2, response, strlen(response), 0);

    closesocket(handlerSocket2);
    closesocket(serverSocket);
}

void startClient() {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create client socket." << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(SERVER_PORT);

    /*if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        return;
    }*/
    ConnectAsyncBlocking(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    const char* message = "Hello from client!";
    send(clientSocket, message, strlen(message), 0);

    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        std::cout << "Received message from server: " << std::string(buffer, 0, bytesReceived) << std::endl;
    }

    std::cout << "Disconnecting" << std::endl;
    disconnect(clientSocket, TF_REUSE_SOCKET);

    std::cout << "Reconnecting" << std::endl;

    /*if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "ReConnect failed." << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        return;
    }*/
    ConnectAsyncBlocking(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    send(clientSocket, message, strlen(message), 0);

    bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        std::cout << "Received message from server: " << std::string(buffer, 0, bytesReceived) << std::endl;
    }

    closesocket(clientSocket);
}