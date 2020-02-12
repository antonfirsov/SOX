#include <iostream> 
#include <cstdlib>
#include <cstring>

#include "EpollServer.hpp"

void DoStuff() {
    
    EpollServer server(AF_INET6, "fe80::ec50:fb40:898f:3795", 11000);
    server.Initialize();
    //server.HandleRequests();

    server.BeginHandlingRequests();

    PressEnter2("abort server");
    std::cout << " aborting... ";
    server.EndHandlingRequests();
    std::cout << "DONE." << std::endl;
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