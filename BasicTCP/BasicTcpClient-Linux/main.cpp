#include <iostream> 
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h> 

void PressEnterPlz() {
    std::cout << "Press Enter Mothafucka!" << std::endl;
    char dummy[128];
    std::cin.getline(dummy, 128);
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s == -1) {
        std::cout << "Could not create socket :((((((" << std::endl;
        return 1;
    }

    if (close(s) == -1) {
        std::cout << "Failed to close the socket :(((((" << std::endl;
    }

    PressEnterPlz();

    return 0;
}