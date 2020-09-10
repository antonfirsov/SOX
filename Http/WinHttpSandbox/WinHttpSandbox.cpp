// WinHttpSandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include "Common.hpp"

#pragma comment (lib, "winhttp.lib")

void ConfigureSession(HINTERNET hSession) {
    DWORD dwHttp2Option = WINHTTP_PROTOCOL_FLAG_HTTP2;
    TRYNZ(
        WinHttpSetOption(hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &dwHttp2Option, sizeof(dwHttp2Option))
    );

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

int main()
{
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL  bResults = FALSE;

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    try {
        // Use WinHttpOpen to obtain a session handle.
        hSession = TRYNZ(
            WinHttpOpen(L"Test!", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)
        );

        ConfigureSession(hSession);

        hConnect = TRYNZ(
            WinHttpConnect(hSession, L"motherfuckingwebsite.com", INTERNET_DEFAULT_HTTPS_PORT, 0)
        );

        hRequest = TRYNZ(
            WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
        );

        ConfigureRequest(hRequest);

        TRYNZ(
            WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0,
                0, 0)
        );

        TRYNZ(
            WinHttpReceiveResponse(hRequest, NULL)
        );

        
        std::stringstream output;

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

        std::cout << output.str() << std::endl;
    }
    catch (const OsError& error) {
        std::cout << error.what() << std::endl;
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    std::cout << "LOLOLOLOLOLOL" << std::endl;
}