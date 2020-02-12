#pragma once

#include <iostream> 
#include <cstring>
#include <sstream> 

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
        std::stringstream ss;
        ss << opName << " failed! errno: " << err;
        throw std::runtime_error(ss.str());
    }
    return fd;
}

template<typename F>
void TryStuffExpectZero(const char* opName, F&& lambda) {
    int err = lambda();

    if (err != 0) {
        std::stringstream ss;
        ss << opName << " failed! Error code: " << err;
        throw std::runtime_error(ss.str());
    }
}

#define TRY( operation ) TryStuff( #operation , [&]() { return operation; })
#define TRYZ( operation ) TryStuffExpectZero( #operation, [&]() { return operation; })