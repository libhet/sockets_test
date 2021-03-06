#include "my_sockets.h"

SocketAddress::SocketAddress(uint32_t inAddress, uint16_t inPort)
{
    GetAsSockAddrIn()->sin_family = AF_INET;
    GetAsSockAddrIn()->sin_addr.S_un.S_addr = htonl(inAddress);
    GetAsSockAddrIn()->sin_port = htons(inPort);
}

SocketAddress::SocketAddress(const sockaddr &inSockAddr)
{
    memcpy(&mSockAddr, &inSockAddr, sizeof( sockaddr) );
}

sockaddr_in *SocketAddress::GetAsSockAddrIn()
{return reinterpret_cast<sockaddr_in*>( &mSockAddr );}

SocketAddressPtr SocketAddressFactory::CreateIPv4FromString(const std::string &inString)
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

SocketAddressPtr SocketAddressFactory::CreateEmptyIPv4(){
    auto toRet = std::make_shared< SocketAddress >(0, 0);
    return toRet;
}


int UDPSocket::Bind(const SocketAddress& inBindAddress)
{
    int err = bind(mSocket, &inBindAddress.mSockAddr,
                   inBindAddress.GetSize());
    if(err != 0)
    {
        SocketUtil::ReportError(L"UDPSocket::Bind");
        return SocketUtil::GetLastError();
    }
    return NO_ERROR;
}

int UDPSocket::SendTo(const void* inData, int inLen,
                      const SocketAddress& inTo)
{
    int byteSentCount = sendto( mSocket,
                                static_cast<const char*>( inData),
                                inLen,
                                0, &inTo.mSockAddr, inTo.GetSize());
    if(byteSentCount >= 0)
    {
        return byteSentCount;
    }
    else
    {
        // вернуть код ошибки как отрицательное число
        SocketUtil::ReportError(L"UDPSocket::SendTo");
        return -SocketUtil::GetLastError();
    }
}
int UDPSocket::ReceiveFrom(void* inBuffer, int inLen,
                           SocketAddress& outFrom)
{
    int fromLength = outFrom.GetSize();
    int readByteCount = recvfrom(mSocket,
                                 static_cast<char*>(inBuffer),
                                 inLen,
                                 0, &(outFrom.mSockAddr),
                                 &fromLength);
    if(readByteCount >= 0)
    {
        return readByteCount;
    }
    else
    {
        SocketUtil::ReportError(L"UDPSocket::ReceiveFrom");
        return -SocketUtil::GetLastError();
    }
}
UDPSocket::~UDPSocket()
{
    closesocket(mSocket);
}

void SocketUtil::ReportError(const std::wstring &error) {
    std::wcout << error << std::endl;
    std::cout << SocketUtil::GetLastErrorText() << std::endl;
}

int SocketUtil::GetLastError() { return WSAGetLastError();}

std::string SocketUtil::GetLastErrorText() {
    char buffer[1024] = {0};
    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
                   NULL,                // lpsource
                   WSAGetLastError (),                 // message id
                   MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),    // languageid
                   buffer,              // output buffer
                   sizeof (buffer),     // size of msgbuf, bytes
                   NULL);               // va_list of arguments

    return std::string(buffer);
}

UDPSocketPtr SocketUtil::CreateUDPSocket(SocketAddressFamily inFamily)
{
    SOCKET s = socket(inFamily, SOCK_DGRAM, IPPROTO_UDP);
    if(s != INVALID_SOCKET)
    {
        return UDPSocketPtr(new UDPSocket(s));
    }
    else
    {
        ReportError(L"SocketUtil::CreateUDPSocket");
        return nullptr;
    }
}

TCPSocketPtr SocketUtil::CreateTCPSocket(SocketAddressFamily inFamily)
{
    SOCKET s = socket(inFamily, SOCK_STREAM, IPPROTO_TCP);
    if(s != INVALID_SOCKET)
    {
        return TCPSocketPtr(new TCPSocket(s));
    }
    else
    {
        ReportError(L"SocketUtil::CreateUDPSocket");
        return nullptr;
    }
}

fd_set *SocketUtil::FillSetFromVector(fd_set &outSet, const std::vector<TCPSocketPtr> *inSockets)
{
    if(inSockets)
    {
        FD_ZERO(&outSet);
        for(const TCPSocketPtr& socket : *inSockets)
        {
            FD_SET(socket->mSocket, &outSet);
        }
        return &outSet;
    }
    else
    {
        return nullptr;
    }
}

void SocketUtil::FillVectorFromSet(std::vector<TCPSocketPtr> *outSockets, const std::vector<TCPSocketPtr> *inSockets, const fd_set &inSet)
{
    if(inSockets && outSockets)
    {
        outSockets->clear();
        for(const TCPSocketPtr& socket : *inSockets)
        {
            if(FD_ISSET(socket->mSocket, &inSet))
            {
                outSockets->push_back(socket);
            }
        }
    }
}

int SocketUtil::Select(const std::vector<TCPSocketPtr> *inReadSet, std::vector<TCPSocketPtr> *outReadSet, const std::vector<TCPSocketPtr> *inWriteSet, std::vector<TCPSocketPtr> *outWriteSet, const std::vector<TCPSocketPtr> *inExceptSet, std::vector<TCPSocketPtr> *outExceptSet)
{
    fd_set read, write, except;
//    memset(&read, 0 , sizeof(fd_set));
//    memset(&write, 0 , sizeof(fd_set));
//    memset(&except, 0 , sizeof(fd_set));
    fd_set *readPtr = FillSetFromVector(read, inReadSet);
    fd_set *writePtr = FillSetFromVector(write, inWriteSet);
    fd_set *exceptPtr = FillSetFromVector(except, inExceptSet);
    timeval time;
    time.tv_sec = 0;
    time.tv_usec = 500;
    int toRet = select(FD_SETSIZE, readPtr, writePtr, exceptPtr, nullptr);
    if(toRet > 0)
    {
        FillVectorFromSet(outReadSet, inReadSet, read);
        FillVectorFromSet(outWriteSet, inWriteSet, write);
        FillVectorFromSet(outExceptSet, inExceptSet, except);
    }
    return toRet;
}

TCPSocket::~TCPSocket()
{
    closesocket(mSocket);
}

int TCPSocket::Connect(const SocketAddress& inAddress)
{
    int err = connect(mSocket, &inAddress.mSockAddr, inAddress.GetSize());
    if(err < 0)
    {
        SocketUtil::ReportError(L"TCPSocket::Connect");
        return -SocketUtil::GetLastError();
    }
    return NO_ERROR;
}

int TCPSocket::Bind(const SocketAddress &inToAddress)
{
    int err = bind(mSocket, &inToAddress.mSockAddr,
                   inToAddress.GetSize());
    if(err != 0)
    {
        SocketUtil::ReportError(L"TCPSocket::Bind");
        return SocketUtil::GetLastError();
    }
    return NO_ERROR;
}

int TCPSocket::Listen(int inBackLog)
{
    int err = listen(mSocket, inBackLog);
    if(err < 0)
    {
        SocketUtil::ReportError(L"TCPSocket::Listen");
        return -SocketUtil::GetLastError();
    }

    return NO_ERROR;
}

TCPSocketPtr TCPSocket::Accept(SocketAddress& inFromAddress)
{
    int length = inFromAddress.GetSize();
    SOCKET newSocket = accept(mSocket, &inFromAddress.mSockAddr, &length);
    if(newSocket != INVALID_SOCKET)
    {
        return TCPSocketPtr(new TCPSocket( newSocket));
    }
    else
    {
        SocketUtil::ReportError(L"TCPSocket::Accept");
        return nullptr;
    }
}

int TCPSocket::Send(const void* inData, int inLen)
{
    int bytesSentCount = send(mSocket,
                              static_cast<const char*>(inData ),
                              inLen, 0);
    if(bytesSentCount < 0 )
    {
        SocketUtil::ReportError(L"TCPSocket::Send");
        return -SocketUtil::GetLastError();
    }
    return bytesSentCount;
}

int TCPSocket::Receive(void* inData, int inLen)
{
    int bytesReceivedCount = recv(mSocket,
                                  static_cast<char*>(inData), inLen, 0);
    if(bytesReceivedCount < 0)
    {
        SocketUtil::ReportError(L"TCPSocket::Receive");
        return -SocketUtil::GetLastError();
    }
    return bytesReceivedCount;
}

int TCPSocket::SetOpt(int level, int optname, const char *optval, int optlen)
{
    return setsockopt(mSocket, level, optname, optval, optlen);
}
