#include "tcpclient.h"

TCPClient::TCPClient(Socket& socket):socket(std::move(socket))
{
    transfer.setRecieveFunction([&](Buffer& buff, size_t size)
    {
        return this->socket.Recieve(buff, size);
    });
    transfer.setSendFunction([&](Buffer& buff, size_t size)
    {
        return this->socket.Send(buff, size);
    });
}

OperationResult TCPClient::Send(size_t size, Buffer& buff, bool confirm)
{
    if (confirm)
        transfer.Start(TransferType::Send, size, genericConfirmCondition);
    else
        transfer.Start(TransferType::Send, size, false);
    return transfer.Execute(buff);
}

OperationResult TCPClient::Send(size_t size, bool confirm)
{
    return Send(size, state->buff, confirm);
}

OperationResult TCPClient::Recieve(size_t size, Buffer& buff, bool confirm)
{
    if (confirm)
        transfer.Start(TransferType::Recieve, size, genericConfirmCondition);
    else
        transfer.Start(TransferType::Recieve, size, false);
    return transfer.Execute(buff);
}

OperationResult TCPClient::Recieve(size_t size, bool confirm)
{
    return Recieve(size, state->buff, confirm);
}

OperationResult TCPClient::Peek(size_t size, Buffer &buff)
{
    return this->socket.Recieve(buff, size, true);
}

OperationResult TCPClient::Peek(size_t size)
{
    return Peek(size, state->buff);
}

Socket& TCPClient::getSocket()
{
    return socket;
}

bool TCPClient::isReachable()
{
    return socket.IsConnected(false);
}

void TCPClient::endTransfer()
{
    transfer.End();
}
