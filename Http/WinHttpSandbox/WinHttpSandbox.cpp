// WinHttpSandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <chrono>
#include <thread>

#include <windows.h>
#include <winhttp.h>
#include <mstcpip.h>
#include <Wincrypt.h>

#include "Common.hpp"


#pragma comment (lib, "winhttp.lib")

void Print(const tcp_keepalive& ka) {
    std::cout << "onoff: " << ka.onoff << "keepaliveinterval: " << ka.keepaliveinterval << "keepalivetime: " << ka.keepalivetime << std::endl;
}

//void Print(const TCP_INFO_v1 tcpInfo& tcpInfo) {
// 
//}


void ConfigureSession(HINTERNET hSession) {
    DWORD dwHttp2Option = WINHTTP_PROTOCOL_FLAG_HTTP2;
    TRYNZ(
        WinHttpSetOption(hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &dwHttp2Option, sizeof(dwHttp2Option))
    );
    
    //Print(ka);

    tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepaliveinterval = 2000;
    ka.keepalivetime = 2000;

    TRYNZ(
        WinHttpSetOption(hSession, WINHTTP_OPTION_TCP_KEEPALIVE, &ka, sizeof(ka))
    );
}


ULONG64 QueryConnectionTime(HINTERNET hRequest) {
    TCP_INFO_v1 tcpInfo = {};
    DWORD size = sizeof(tcpInfo);
    try {
        TRYNZ(
            WinHttpQueryOption(hRequest, WINHTTP_OPTION_CONNECTION_STATS_V1, &tcpInfo, &size)
        );

        std::cout << "ConnectionTimeMs: " << tcpInfo.ConnectionTimeMs << std::endl;
        return tcpInfo.ConnectionTimeMs;
    }
    catch (const OsError& error) {
        std::cout << error.what() << std::endl;
        return 0;
    }
}

void ExpireConnection(HINTERNET hRequest) {
    try {

        TRYNZ(
            WinHttpSetOption(hRequest, WINHTTP_OPTION_EXPIRE_CONNECTION, nullptr, 0)
        );
        
    }
    catch (const OsError& error) {
        std::cout << error.what() << std::endl;
    }
}

void ConfigureRequest(HINTERNET hRequest) {
    DWORD dwAutoLogonHigh = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    TRYNZ(
        WinHttpSetOption(hRequest,
            WINHTTP_OPTION_AUTOLOGON_POLICY,
            &dwAutoLogonHigh,
            sizeof(dwAutoLogonHigh))
    );    
}

std::string SendRequest(HINTERNET hSession) {
    HINTERNET hConnect = NULL, hRequest = NULL;
    DWORD dwSize = 0;
    std::stringstream output;

    try {
        hConnect = TRYNZ(
            WinHttpConnect(hSession, L"corefx-net-http2.azurewebsites.net", INTERNET_DEFAULT_HTTPS_PORT, 0)
        );

        hRequest = TRYNZ(
            WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
        );

        ConfigureRequest(hRequest);

        /*std::cout << "before send ";
        QueryConnectionTime(hRequest);*/

        TRYNZ(
            WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0,
                0, 0)
        );

        std::cout << "after send ";
        auto time = QueryConnectionTime(hRequest);
        if (time > 2000) {
            std::cout << "EXPIRING!!!" << std::endl;
            ExpireConnection(hRequest);
        }

        TRYNZ(
            WinHttpReceiveResponse(hRequest, NULL)
        );

        do {
            TRYNZ(WinHttpQueryDataAvailable(hRequest, &dwSize));

            DWORD bufferSize = dwSize + 1;

            auto buffer = new char[bufferSize]();

            DWORD dwDownloaded = 0;
            TRYNZ(
                WinHttpReadData(hRequest, (LPVOID)buffer, dwSize, &dwDownloaded)
            );

            output << std::string(buffer, (size_t)dwDownloaded);

        } while (dwSize > 0);   
    }
    catch (const OsError& error) {
        std::cout << error.what() << std::endl;
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);

    return output.str();
}

int main()
{
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL  bResults = FALSE;
    HINTERNET hSession = NULL;

    try {
        // Use WinHttpOpen to obtain a session handle.
        hSession = TRYNZ(
            WinHttpOpen(L"Test!", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)
        );

        ConfigureSession(hSession);

        std::string response = SendRequest(hSession);
        std::cout << response << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        SendRequest(hSession);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        SendRequest(hSession);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        SendRequest(hSession);
    }
    catch (const OsError& error) {
        std::cout << error.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(30));

    if (hSession) WinHttpCloseHandle(hSession);

    std::cout << "LOLOLOLOLOLOL" << std::endl;
}