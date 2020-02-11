#pragma once

#include <iostream> 
#include <cstring>

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

#define TRY( opName, operation ) TryStuff(opName, [&]() { return operation; })