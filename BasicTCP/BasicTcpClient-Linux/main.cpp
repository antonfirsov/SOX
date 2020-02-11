#include <iostream> 
#include <cstring>

#include "Client.hpp"

void DoStuff() {
    Client client("172.17.204.241", 11000);
    client.CreateSocket();

    PressEnter2("connect");
    std::cout << "connecting ...";
    client.Connect();
    std::cout << "connected!" << std::endl;
    
    client.DoSmalltalk();
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