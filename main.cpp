#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <sstream>

#include <memory>
#include "my_sockets.h"


void sender(std::string message) {

    WSAData wsa_data;
    WSAStartup(WINSOCK_VERSION, &wsa_data);

    char buffer[1024] = {0};
    memcpy(buffer, message.c_str(), message.size());
    auto tcp_socket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
    auto address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    tcp_socket->Connect(*address);

//    while(true) {
        char response_buffer[1024] = {0};
        memcpy(buffer, message.c_str(), message.size());
        tcp_socket->Send(buffer, message.size());
        tcp_socket->Receive(response_buffer, 1024);
        std::cout << "Answer: " << response_buffer << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

//    }
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


void ProcessNewClient(const TCPSocketPtr & _socket, const SocketAddress &address) {
    std::stringstream response;
    std::stringstream response_body;

    response_body << "Welcome!!\n";

    response << "HTTP/1.1 200 OK\r\n"
            << "Version: HTTP/1.1\r\n"
            << "Content-Type: text/html; charset=utf-8\r\n"
            << "Content-Length: " << response_body.str().length()
            << "\r\n\r\n"
            << response_body.str();

    _socket->Send(response.str().c_str(), response.str().size());
}

void ProcessDataFromClient(const TCPSocketPtr & _socket, char * segment, int size) {
    std::stringstream response;
    std::stringstream response_body;

    response_body << "Welcome!\n";

    response << "HTTP/1.1 200 OK\r\n"
            << "Version: HTTP/1.1\r\n"
            << "Content-Type: text/html; charset=utf-8\r\n"
            << "Content-Length: " << response_body.str().length()
            << "\r\n\r\n"
            << response_body.str();

    _socket->Send(response.str().c_str(), response.str().size());
}


void server() {
    WSAData wsa_data;
    WSAStartup(WINSOCK_VERSION, &wsa_data);

    TCPSocketPtr listenSocket = SocketUtil::CreateTCPSocket(INET);
    SocketAddressPtr receivingAddres = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:80");
    if( listenSocket->Bind(*receivingAddres ) != NO_ERROR)
    {
        return;
    }
    listenSocket->Listen();

    std::vector<TCPSocketPtr> readBlockSockets;

    readBlockSockets.push_back(listenSocket);

    std::vector<TCPSocketPtr> readableSockets;
    std::vector<TCPSocketPtr> writableSockets;

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if(SocketUtil::Select(&readBlockSockets, &readableSockets,
                              &readBlockSockets, &writableSockets,
                              nullptr, nullptr))
        {
            // получен пакет — обойти сокеты...
            for(const TCPSocketPtr& socket : readableSockets)
            {
                if(socket == listenSocket)
                {
                    // это сокет, принимающий запросы на соединение,
                    // принять новое соединение
                    SocketAddress newClientAddress;
                    auto newSocket = listenSocket->Accept(newClientAddress);
//                    char val[512] = {0};
//                    newSocket->SetOpt(SOL_SOCKET, SO_KEEPALIVE, val, 4);
                    readBlockSockets.push_back(newSocket);
                    ProcessNewClient(newSocket, newClientAddress);
                }
                else
                {
                    // это обычный сокет — обработать данные...
                    char segment[1024];
                    int dataReceived =
                            socket->Receive( segment, 1024 );
                    if(dataReceived > 0)
                    {
                        ProcessDataFromClient( socket, segment,
                                               dataReceived);
                    }
                }
            }

            for(const TCPSocketPtr& socket : writableSockets)
            {
                if(socket == listenSocket)
                {
                    // это сокет, принимающий запросы на соединение,
                    // принять новое соединение
                    SocketAddress newClientAddress;
                    auto newSocket = listenSocket->Accept(newClientAddress);
                    readBlockSockets.push_back(newSocket);
                    ProcessNewClient(newSocket, newClientAddress);
                }
                else
                {
                    // это обычный сокет — обработать данные...
                    char segment[1024];
                    int dataReceived =
                            socket->Receive( segment, 1024 );
                    if(dataReceived > 0)
                    {
                        ProcessDataFromClient( socket, segment,
                                               dataReceived);
                    }
                }
            }
        }
    }
}



int main()
{
    WSAData wsa_data;
    WSAStartup(WINSOCK_VERSION, &wsa_data);

//    server();


    std::thread t1(server);
//    std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    std::thread t2(sender, "I'm secod!");
//    std::thread t3(sender, "I'm third!");
//    std::thread t4(sender, "I'm fourth!");

    t1.join();
//    t2.join();
//    t3.join();
//    t4.join();



    WSACleanup();
    return 0;
}
