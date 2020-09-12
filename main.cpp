#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <memory>

// not windows
// #include <sys/socket.h> // Для работы с сокетами
// #include <netinet/in.h> // Функции для ip4
// #include <arpa/inet.h>  // Функции преобразования адресов
// #include <netdb.h>   // Для разрешения адресов


// Для корректной работы freeaddrinfo в MinGW
// Подробнее: http://stackoverflow.com/a/20306451
//#define _WIN32_WINNT 0x501

#include <WinSDKVer.h>
#define _WIN32_WINNT _WIN32_WINNT_WIN7  // Устанавливаем минимальную поддерживаемую версию Windows
#include <sdkddkver.h>

#include <WinSock2.h>
#include <Ws2tcpip.h>

#define WIN32_LEAN_AND_MEAN // Has to be before include Windows.h


using std::cerr;
//using namespace std;


class SocketAddress
{
public:
    SocketAddress(uint32_t inAddress, uint16_t inPort)
    {
        GetAsSockAddrIn()->sin_family = AF_INET;
        GetAsSockAddrIn()->sin_addr.S_un.S_addr = htonl(inAddress);
        GetAsSockAddrIn()->sin_port = htons(inPort);
    }

    SocketAddress(const sockaddr& inSockAddr)
    {
        memcpy(&mSockAddr, &inSockAddr, sizeof( sockaddr) );
    }

    size_t GetSize() const {return sizeof( sockaddr );}

private:
    sockaddr mSockAddr;

    sockaddr_in* GetAsSockAddrIn()
    {return reinterpret_cast<sockaddr_in*>( &mSockAddr );}

};

typedef std::shared_ptr<SocketAddress> SocketAddressPtr;



class SocketAddressFactory
{
public:
    static SocketAddressPtr CreateIPv4FromString(const std::string& inString)
    {
        auto pos = inString.find_last_of(':');
        std::string host, service;
        if(pos != std::string::npos)
        {
            host = inString.substr(0, pos);
            service = inString.substr(pos + 1);
        }
        else
        {
            host = inString;
            // использовать порт по умолчанию...
            service = "0";
        }
        addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        addrinfo* result;
        int error = getaddrinfo(host.c_str(), service.c_str(),
                                &hint, &result);
        if(error != 0 && result != nullptr)
        {
            freeaddrinfo(result);
            return nullptr;
        }
        while(!result->ai_addr && result->ai_next)
        {
            result = result->ai_next;
        }
        if(!result->ai_addr)
        {
            freeaddrinfo(result);
            return nullptr;
        }
        auto toRet = std::make_shared< SocketAddress >(*result->ai_addr);
        freeaddrinfo(result);
        return toRet;
    }
};





/*
 *
struct sockaddr_in {
    short	sin_family;
    u_short	sin_port;
    struct in_addr	sin_addr;
    char	sin_zero[8];
};
*/


void handle_error(int error) {
    if(error == SOCKET_ERROR) {
        char buffer[1024] = {0};
        FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
                       NULL,                // lpsource
                       WSAGetLastError (),                 // message id
                       MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),    // languageid
                       buffer,              // output buffer
                       sizeof (buffer),     // size of msgbuf, bytes
                       NULL);               // va_list of arguments
        std::cout << buffer << std::endl;
        WSACleanup();
    }


}





void sender() {
    std::cout << "sender" << std::endl;

    char buffer[256] = {0};

    const char * message = "Messge!";

    memcpy(buffer, message, 7);

    WSAData wsa_data;

    int function_result = WSAStartup(WINSOCK_VERSION, &wsa_data);
    handle_error(function_result);

    sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(1010);
    send_addr.sin_addr.S_un.S_un_b.s_b1 = 127;
    send_addr.sin_addr.S_un.S_un_b.s_b2 = 0;
    send_addr.sin_addr.S_un.S_un_b.s_b3 = 0;
    send_addr.sin_addr.S_un.S_un_b.s_b4 = 1;

    auto soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    function_result = sendto(soc, buffer, 10, 0, (sockaddr*)&send_addr, (int)sizeof(sockaddr_in));
    handle_error(function_result);


    shutdown(soc, SD_BOTH);
    closesocket(soc);
}

void receiver() {

    std::cout << "reciever" << std::endl;

    /*
     * Buffer for recieved data
     */
    char buffer[256] = {0};

    WSAData wsa_data;

    int function_result = WSAStartup(WINSOCK_VERSION, &wsa_data);
    handle_error(function_result);

    sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(1010);
    my_addr.sin_addr.S_un.S_un_b.s_b1 = 127;
    my_addr.sin_addr.S_un.S_un_b.s_b2 = 0;
    my_addr.sin_addr.S_un.S_un_b.s_b3 = 0;
    my_addr.sin_addr.S_un.S_un_b.s_b4 = 1;
    auto soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    function_result = bind(soc, (sockaddr*)&my_addr, (int)sizeof(sockaddr_in));
    handle_error(function_result);

    sockaddr_in soc_addr_from;
    soc_addr_from.sin_family = AF_INET;
    soc_addr_from.sin_port = htons(1010);
    soc_addr_from.sin_addr.S_un.S_un_b.s_b1 = 127;
    soc_addr_from.sin_addr.S_un.S_un_b.s_b2 = 0;
    soc_addr_from.sin_addr.S_un.S_un_b.s_b3 = 0;
    soc_addr_from.sin_addr.S_un.S_un_b.s_b4 = 1;
    int fromlen = sizeof (soc_addr_from);

    function_result = recvfrom(soc, buffer, 255, 0, (sockaddr*)&soc_addr_from, &fromlen);
    handle_error(function_result);

    std::cout << buffer << std::endl;

    shutdown(soc, SD_BOTH);
    closesocket(soc);
}


int main()
{
    //    // Windows Only --- BEGIN ---
    //    // Windows Sockets API
    //    WSAData wsa_data;

    //    int result = WSAStartup(WINSOCK_VERSION, &wsa_data);

    //    if(result == SOCKET_ERROR) {
    //        std::cout << WSAGetLastError() << std::endl;
    //    }
    //    // Windows Only --- END ---

    //    sockaddr_in myAddr;
    //    myAddr.sin_family = AF_INET;
    //    myAddr.sin_port = htons(80);
    //    myAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    ////    InetPton(AF_INET, "65.254.248.180", &myAddr.sin_addr);
    ////    auto a = SocketAddressFactory::CreateIPv4FromString("www.google.com");

    //    auto a = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


    //    /// Для сервера указываем точный адрес и порт
    //    /// Если установить порт   0 то порт выберется автоматически
    //    result = bind(a, (sockaddr*)&myAddr, (int)sizeof(myAddr.sin_addr));
    //    if(result == SOCKET_ERROR) {
    //        std::cout << WSAGetLastError() << std::endl;
    //    }

    //    /// Передача UDP
    //    result = sendto(a, )

    //    shutdown(a, SD_BOTH);
    //    closesocket(a);

    //    // Windows Only --- BEGIN ---
    //    result = WSACleanup();
    //    if(result == SOCKET_ERROR) {
    //        std::cout << WSAGetLastError() << std::endl;
    //    }

    // Windows Only --- END ---


    std::thread t1(receiver);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread t2(sender);

    t1.join();
    t2.join();
}
