#include <iostream> 
#include <cstdlib>
#include <cstring>
#include <memory>

//#include "EpollServer.hpp"
#include "UringServer.hpp"

void DoStuff() {
    
    std::unique_ptr<UringServer> server = std::make_unique<UringServer>(AF_INET, "172.17.99.105", 11002);
    server->Initialize();
    server->BeginHandlingRequests();
    //server->HandleRequests();

    PressEnter2("abort server");
    std::cout << " aborting... " << std::endl;
    server->EndHandlingRequests();
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