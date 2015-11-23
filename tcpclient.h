#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "client.h"
#include "socket.h"
#include "transfer.h"


class TCPClient: public Client
{
    Socket socket;
    Transfer transfer;
public:
    TCPClient(Socket& socket);
    OperationResult Send(size_t size, Buffer& buff, bool confirm = true);
    OperationResult Send(size_t size, bool confirm = true);
    OperationResult Recieve(size_t size, Buffer& buff, bool confirm = true);
    OperationResult Recieve(size_t size, bool confirm = true);
    OperationResult Peek(size_t size, Buffer& buff);
    OperationResult Peek(size_t size);
    Socket& getSocket();
    bool isReachable();
    void endTransfer();
};

#endif // TCPCLIENT_H
