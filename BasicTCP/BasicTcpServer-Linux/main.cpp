#include <iostream> 
#include <cstdlib>
#include <cstring>

#include "EpollServer.hpp"

void DoStuff() {
    
    EpollServer server("172.17.204.253", 11000);
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