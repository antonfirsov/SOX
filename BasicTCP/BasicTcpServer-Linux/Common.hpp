#pragma once

#include <iostream> 
#include <cstring>
#include <sstream> 

class OsError : public std::runtime_error {
    int _errorCode;
    
    static const std::string GetMessage(const char* opName, int errorCode) {
        std::stringstream ss;
        ss << opName << " failed! return val / errno: " << errorCode;
        return ss.str();
    }

public:
    OsError(const char* opName, int errorCode)
        : std::runtime_error(GetMessage(opName, errorCode)), _errorCode(errorCode)
    {
    }

    const int ErrorCode() const {
        return _errorCode;
    }
};

void PressEnter2(const char* opName) {
    std::cout << "Press Enter to " << opName << '!' << std::endl;
    char dummy[128];
    std::cin.getline(dummy, 128);
}

template<typename F>
auto TryStuff(const char* opName, F&& lambda) -> decltype(lambda()) {
    auto fd = lambda();
    if (fd < 0) {
        int err = errno;
        throw OsError(opName, err);
    }
    return fd;
}

template<typename F>
void TryStuffExpectZero(const char* opName, F&& lambda) {
    int err = lambda();

    if (err != 0) {
        throw OsError(opName, err);
    }
}

#define TRY( operation ) TryStuff( #operation , [&]() { return operation; })
#define TRYZ( operation ) TryStuffExpectZero( #operation, [&]() { return operation; })