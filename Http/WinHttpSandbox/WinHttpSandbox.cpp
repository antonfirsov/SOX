// WinHttpSandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include "Common.hpp"

#pragma comment (lib, "winhttp.lib")

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

        hConnect = TRYNZ(
            WinHttpConnect(hSession, L"motherfuckingwebsite.com", INTERNET_DEFAULT_HTTPS_PORT, 0)
        );

        hRequest = TRYNZ(
            WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
        );

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

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
