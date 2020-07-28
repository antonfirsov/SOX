#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define _GET_LAST_ERROR WSAGetLastError()
#include "Common.hpp"


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

std::string GetIPString(const SOCKADDR_IN& a) {
    char hostname[NI_MAXHOST];
    char servInfo[NI_MAXSERV];

    std::stringstream ss;

    TRYZ(getnameinfo((SOCKADDR*)&a, sizeof(a), hostname, NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV));
    ss << hostname << ':' << servInfo;

    return ss.str();
}

void DoBind4(SOCKET s, const char* ip = "0.0.0.0", int Port = 0) {

    SOCKADDR_IN localEp = {
        .sin_family = AF_INET,
        .sin_port = htons(Port)
    };
    if (inet_pton(AF_INET, ip, &localEp.sin_addr) != 1) {
        throw std::runtime_error("inet_pton failed!");
    }

    TRY(bind(s, (SOCKADDR*)&localEp, sizeof(localEp)));

    int size = sizeof(localEp);
    TRYZ(getsockname(s, (SOCKADDR*)&localEp, (int*)&size));

    std::cout << "Bound! " << GetIPString(localEp) << std::endl;

}


struct ProgramArguments {
    std::string EndpointString;
    bool IsUdp;
    bool IsDualMode;

    ProgramArguments() : EndpointString("0.0.0.0:0"), IsUdp(false), IsDualMode(false) {
    }

    ProgramArguments(int ac, const char** av) : ProgramArguments()
    {
        if (ac > 1) {
            EndpointString = av[1];
        }
        if (ac > 2) {
            IsUdp = std::string(av[2]) == "udp";
        }
        if (ac > 3) {
            IsDualMode = std::string(av[3]) == "dual";
        }
    }

    int SockType() const { return IsUdp ? SOCK_DGRAM : SOCK_STREAM; }
    int Protocol() const { return IsUdp ? IPPROTO_UDP : IPPROTO_TCP; }
};

union SocketAddress {
    struct sockaddr         a;
    struct sockaddr_in      a4;
    struct sockaddr_in6     a6;
    struct sockaddr_storage ss;
};


struct SocketInfo {
    SocketAddress CreationAddress;
    SocketAddress LocalEndpoint;

    int SockType;
    int Protocol;
    
    SOCKET Socket;

    ADDRESS_FAMILY Family() const { return CreationAddress.a.sa_family; }

    const socklen_t AddrLen() const {
        return Family() == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    }

    int Port() const {
        const SocketAddress& ep = _bound ? LocalEndpoint : CreationAddress;

        if (Family() == AF_INET) {
            return ep.a4.sin_port;
        }
        else {
            return ep.a6.sin6_port;
        }
    }

    static SocketInfo Create(int sockType, const std::string& epStr = "0.0.0.0:0") {
        size_t commaIdx = epStr.rfind('/');
        std::string addrStr = epStr.substr(0, commaIdx);
        std::string portStr = epStr.substr(commaIdx + 1);

        int port = std::stoi(portStr);

        struct addrinfo* res = NULL;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = sockType;
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

        TRYZ(getaddrinfo(addrStr.c_str(), NULL, &hints, &res));

        if (res == NULL)
        {
            // Failure to resolve 'ip6str'
            throw std::runtime_error("Host not found?");
        }

        SocketInfo result;
        result.SockType = sockType;
        result.Protocol = sockType == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP;

        // We use the first returned entry
        memcpy(&result.CreationAddress.ss, res->ai_addr, res->ai_addrlen);

        freeaddrinfo(res);

        result.SetPort(port);

        return result;
    }

    static SocketInfo Create(const ProgramArguments& args) {
        return Create(args.SockType(), args.EndpointString);
    }

    void CreateSocket() {
        ADDRESS_FAMILY family = Family();
        Socket = socket(family, SockType, Protocol);
        std::cout << "Socket created!" << std::endl;
    }

    ~SocketInfo() {
        if (Socket) {
            closesocket(Socket);
        }
    }

    void Bind() {
        TRY(bind(Socket, &CreationAddress.a, AddrLen()));

        int size = AddrLen();
        TRYZ(getsockname(Socket, &LocalEndpoint.a, &size));

        std::cout << "Bound! " << GetIPString() << std::endl;
    }

    std::string GetIPString() {
        char addrStr[INET6_ADDRSTRLEN];

        if (Family() == AF_INET) {
            inet_ntop(AF_INET, &(LocalEndpoint.a4.sin_addr), addrStr, INET_ADDRSTRLEN);
        }
        else {
            inet_ntop(AF_INET6, &(LocalEndpoint.a6.sin6_addr), addrStr, INET6_ADDRSTRLEN);
        }
        
        std::stringstream ss;
        ss << addrStr << '/' << Port();

        return ss.str();
    }

private:
    bool _bound;


    void SetPort(int port) {
        if (Family() == AF_INET) {
            CreationAddress.a4.sin_port = port;
        }
        else if (Family() == AF_INET6) {
            CreationAddress.a6.sin6_port = port;
        }
        else {
            throw std::runtime_error("Uknown family.");
        }
    }
};



int main(int ac, const char** av)
{
    InitWinsock();

    ProgramArguments args(ac, av);

    try {
        SocketInfo sock = SocketInfo::Create(args);
        sock.CreateSocket();
        sock.Bind();
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
        WSACleanup();
        return -1;
    }
    
    WSACleanup();
    return 0;
}
