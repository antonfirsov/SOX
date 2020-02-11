#include <iostream> 
#include <cstdlib>
#include <cstring>

#include "BasicServer.hpp"

void DoStuff() {
    
    BasicServer server("172.17.204.253", 11000);
    server.CreateListenerSocket();
    server.HandleRequests();
}

int main()
{
    try {
        DoStuff();
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }

    PressEnter2("close");

    return 0;
}