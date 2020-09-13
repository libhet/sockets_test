#include <iostream>
#include <string>
#include <memory>

#include <WinSDKVer.h>
#define _WIN32_WINNT _WIN32_WINNT_WIN7  // Устанавливаем минимальную поддерживаемую версию Windows
#include <sdkddkver.h>

#include <WinSock2.h>
#include <Ws2tcpip.h>

#define WIN32_LEAN_AND_MEAN // Has to be before include Windows.h


enum SocketAddressFamily
{
 INET = AF_INET,
 INET6 = AF_INET6
};

class UDPSocket;
typedef std::shared_ptr<UDPSocket> UDPSocketPtr;


class SocketUtil {
public:
    static void ReportError(const std::wstring &error);
    static int GetLastError();
    static std::string GetLastErrorText();
    static UDPSocketPtr CreateUDPSocket(SocketAddressFamily inFamily);
};


class SocketAddress
{
public:
    SocketAddress(uint32_t inAddress, uint16_t inPort);

    SocketAddress(const sockaddr& inSockAddr);

    size_t GetSize() const {return sizeof( sockaddr );}

private:
//    friend class SocketUtil;
    friend class UDPSocket;

    sockaddr mSockAddr;

    sockaddr_in* GetAsSockAddrIn();

};

typedef std::shared_ptr<SocketAddress> SocketAddressPtr;



class SocketAddressFactory
{
public:
    static SocketAddressPtr CreateIPv4FromString(const std::string& inString);
    static SocketAddressPtr CreateEmptyIPv4();
};


class UDPSocket
{
public:
    ~UDPSocket();
    int Bind(const SocketAddress& inToAddress);
    int SendTo(const void* inData, int inLen, const SocketAddress& inTo);
    int ReceiveFrom(void* inBuffer, int inLen, SocketAddress& outFrom);
private:
    friend class SocketUtil;
    UDPSocket(SOCKET inSocket) : mSocket(inSocket) {}
    SOCKET mSocket;
};

