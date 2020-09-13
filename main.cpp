#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <memory>
#include "my_sockets.h"


void sender(std::string message) {
    char buffer[1024] = {};
    memcpy(buffer, message.c_str(), message.size());
    auto udp_socket = SocketUtil::CreateUDPSocket(SocketAddressFamily::INET);
    auto address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    udp_socket->SendTo(buffer, message.size(), *address);
}

void receiver() {
    char buffer[1024] = {0};
    auto udp_socket = SocketUtil::CreateUDPSocket(SocketAddressFamily::INET);
    auto address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    udp_socket->Bind(*address);
    udp_socket->ReceiveFrom(buffer, 100, *address);
    std::cout << buffer << std::endl;
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
