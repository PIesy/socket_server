#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "client.h"
#include "socket.h"
#include "transfer.h"

class UDPClient:public Client
{
    Socket& socket;
    unsigned short port;
    std::string ip;
    Transfer transfer;
public:
    UDPClient(Socket& socket, const std::string& ip, unsigned short port);
    OperationResult Send(size_t size, Buffer& buff, bool confirm = true);
    OperationResult Send(size_t size, bool confirm = true);
    OperationResult Recieve(size_t size, Buffer& buff, bool confirm = true);
    OperationResult Recieve(size_t size, bool confirm);
    OperationResult Peek(size_t size, Buffer& buff);
    OperationResult Peek(size_t size);
    Socket& getSocket();
    bool isReachable();
    void endTransfer();
    void setIp(const std::string& ip);
    void setPort(unsigned short port);
};

#endif // UDPCLIENT_H
