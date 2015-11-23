#include "udpclient.h"

UDPClient::UDPClient(Socket& socket, const std::string& ip, unsigned short port):socket(socket)
{
    this->port = port;
    this->ip = ip;
    transfer.setRecieveFunction([&](Buffer& buff, size_t size)
    {
        std::string ip;
        unsigned short port;
        return this->socket.RecieveFrom(buff, size, ip, port);
    });
    transfer.setSendFunction([&](Buffer& buff, size_t size)
    {
        return this->socket.SendTo(buff, size, this->ip, this->port);
    });
}

OperationResult UDPClient::Send(size_t size, Buffer& buff, bool confirm)
{
    if (confirm)
        transfer.Start(TransferType::Send, size, genericConfirmCondition);
    else
        transfer.Start(TransferType::Send, size, false);
    return transfer.Execute(buff);
}

OperationResult UDPClient::Send(size_t size, bool confirm)
{
    return Send(size, state->buff, confirm);
}

OperationResult UDPClient::Recieve(size_t size, Buffer& buff, bool confirm)
{
    if (confirm)
        transfer.Start(TransferType::Recieve, size, genericConfirmCondition);
    else
        transfer.Start(TransferType::Recieve, size, false);
    return transfer.Execute(buff);
}

OperationResult UDPClient::Recieve(size_t size, bool confirm)
{
    return Recieve(size, state->buff, confirm);
}

OperationResult UDPClient::Peek(size_t size, Buffer& buff)
{
    std::string ip;
    unsigned short port;
    return this->socket.RecieveFrom(buff, size, ip, port, true);
}

OperationResult UDPClient::Peek(size_t size)
{
    return Peek(size, state->buff);
}

Socket& UDPClient::getSocket()
{
    return socket;
}

bool UDPClient::isReachable()
{
    return true;
}

void UDPClient::endTransfer()
{
    transfer.End();
}

void UDPClient::setIp(const std::string& ip)
{
    this->ip = ip;
}

void UDPClient::setPort(unsigned short port)
{
    this->port = port;
}
