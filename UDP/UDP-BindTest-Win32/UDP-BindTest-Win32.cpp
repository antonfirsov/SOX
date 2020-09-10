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

std::string GetEndpointString(const SocketAddress& a, ADDRESS_FAMILY af) {
    char addrStr[INET6_ADDRSTRLEN];

    int port;
    if (af == AF_INET) {
        inet_ntop(AF_INET, &(a.a4.sin_addr), addrStr, INET_ADDRSTRLEN);
        port = a.a4.sin_port;
    }
    else {
        inet_ntop(AF_INET6, &(a.a6.sin6_addr), addrStr, INET6_ADDRSTRLEN);
        port = a.a6.sin6_port;
    }


    std::stringstream ss;
    ss << addrStr << '/' << port;

    return ss.str();
}


struct SocketInfo {
    SocketAddress CreationAddress;
    SocketAddress LocalEndpoint;

    int SockType;
    int Protocol;
    bool IsDualMode;
    
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

    static SocketInfo Create(int sockType, bool dual, const std::string& epStr = "0.0.0.0:0") {
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
        result.IsDualMode = dual;

        // We use the first returned entry
        memcpy(&result.CreationAddress.ss, res->ai_addr, res->ai_addrlen);

        freeaddrinfo(res);

        result.SetPort(port);

        return result;
    }

    static SocketInfo Create(const ProgramArguments& args) {
        return Create(args.SockType(), args.IsDualMode, args.EndpointString);
    }

    void CreateSocket() {
        ADDRESS_FAMILY family = Family();
        Socket = socket(family, SockType, Protocol);

        if (IsDualMode) {
            if (family == AF_INET) {
                throw std::runtime_error("IsDualMode can't be used with IPV4!");
            }
            int disable = 0;
            TRY(setsockopt(Socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&disable, sizeof(disable)));
        }

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
        return GetEndpointString(LocalEndpoint, Family());
    }

    void RecvFrom() {
        std::cout << "Receiving ..." << std::endl;

        char buf[128];
        SOCKADDR_IN senderAddress;
        int len = sizeof(senderAddress);
        int received = TRY(recvfrom(Socket, buf, 128, 0, (SOCKADDR*)&senderAddress, &len));
        std::cout << "Received " << received << " bytes" << std::endl;
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

    //ProgramArguments args(ac, av);
    
    try {

        ProgramArguments args1 = {};
        args1.EndpointString = "0.0.0.0/0";
        args1.IsUdp = true;

        SocketInfo sock1 = SocketInfo::Create(args1);
        sock1.CreateSocket();
        sock1.Bind();

        std::stringstream ss;
        //ss << "::ffff:127.0.0.1/" << sock1.Port();
        ss << "::ffff:0.0.0.0/" << sock1.Port();
        //ss << "::/" << sock1.Port();
        //ss << "0.0.0.0/" << sock1.Port();

        ProgramArguments args2 = {};
        args2.EndpointString = ss.str();
        args2.IsUdp = true;
        args2.IsDualMode = true;

        SocketInfo sock2 = SocketInfo::Create(args2);
        sock2.CreateSocket();

        int exclusiveAddressReuseVal;
        int optSize = sizeof(exclusiveAddressReuseVal);
        getsockopt(sock2.Socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&exclusiveAddressReuseVal, &optSize);

        int reuseAddrValue;
        getsockopt(sock2.Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddrValue, &optSize);
        std::cout << "SO_EXCLUSIVEADDRUSE=" << exclusiveAddressReuseVal << " SO_REUSEADDR=" << reuseAddrValue << std::endl;

        sock2.Bind();
        //sock1.RecvFrom();
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
        WSACleanup();
        return -1;
    }
    
    WSACleanup();
    return 0;
}
