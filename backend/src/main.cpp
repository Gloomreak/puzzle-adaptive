#include <iostream>
#include <windows.h>
#include "server/Server.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    std::cout << "=== Game 15 v1.0 ===" << std::endl;
    std::cout << "Adaptive learning" << std::endl << std::endl;
    
    Server server(8080);
    server.start();
    
    return 0;
}