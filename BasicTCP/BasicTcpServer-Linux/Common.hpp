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
    if (fd < 0) throw std::runtime_error(opName + std::string(" failed!"));
    return fd;
}

template<typename F>
void TryStuffExpectZero(const char* opName, F&& lambda) {
    int err = lambda();
    std::stringstream ss;
    ss << opName << " failed! Error code: " << err;
    if (err != 0) throw std::runtime_error(ss.str());
}

#define TRY( opName, operation ) TryStuff(opName, [&]() { return operation; })
#define TRYZ( opName, operation ) TryStuffExpectZero(opName, [&]() { return operation; })