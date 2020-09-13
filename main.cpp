#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <memory>
#include "my_sockets.h"


void sender(std::string message) {
    char buffer[1024] = {0};
    memcpy(buffer, message.c_str(), message.size());
    auto tcp_socket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
    auto address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    tcp_socket->Connect(*address);
    tcp_socket->Send(buffer, message.size());
}

void receiver() {
    char buffer[1024] = {0};
    auto tcp_socket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
    auto address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    auto from_address = SocketAddressFactory::CreateEmptyIPv4();
    tcp_socket->Bind(*address);
    tcp_socket->Listen();
    while(true) {
        auto connection_socket = tcp_socket->Accept(*from_address);
        connection_socket->Receive(buffer, 100);
        std::cout << buffer << std::endl;
    }
}


int main()
{
    WSAData wsa_data;
    WSAStartup(WINSOCK_VERSION, &wsa_data);

    std::thread t1(receiver);
//    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread t2(sender, "I'm secod!");
    std::thread t3(sender, "I'm third!");
    std::thread t4(sender, "I'm fourth!");

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
