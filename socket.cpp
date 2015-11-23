#include "socket.h"
#include "helpers.h"
#include <iostream>

int getMaxSocketDescriptorValue(const std::vector<Socket*>& first,
                                const std::vector<Socket*>& second,
                                const std::vector<Socket*>& third);

SelectResult buildSelectResult(bool ready, fd_set& r, fd_set& w, fd_set& e,
                               const std::vector<Socket*>& r_original,
                               const std::vector<Socket*>& w_original,
                               const std::vector<Socket*>& e_original);

Socket::Socket() {}

Socket::Socket(SocketType type, const std::string& ip, unsigned short port)
{
    if (asBool(Create(type)))
        if (ip != "" && port)
            Bind(ip, port);
}

Socket::Socket(const sockets::Socket& socket)
{
    isValid = socket.isValid;
    this->socket = socket;
}

Socket::Socket(Socket&& rhs)
{
    operator =(std::move(rhs));
}

Socket& Socket::operator=(Socket&& rhs)
{
    this->~Socket();
    socket = rhs.socket;
    isValid = rhs.isValid;
    isBound = rhs.isBound;
    ip = rhs.ip;
    port = rhs.port;
    rhs.isValid = false;
    return *this;
}

Socket& Socket::operator=(const sockets::Socket& rhs)
{
    isValid = rhs.isValid;
    socket = rhs;
    return *this;
}

Socket::~Socket()
{
    if (socket.isValid)
    {
        std::cerr << "Closing socket" << std::endl;
        isValid = false;
        sockets::closeSocket(socket);
    }
}

OperationResult Socket::Bind(const std::string& ip, unsigned short port)
{
    this->ip = ip;
    this->port = port;
    isValid = sockets::bindSocket(socket, {port, ip, sockets::INET});
    isBound = isValid;
    return fromBool(isValid);
}

OperationResult Socket::Listen(int maxClientsNumber)
{
    return fromBool(sockets::listen(socket, maxClientsNumber));
}

OperationResult Socket::Create(SocketType type)
{
    socket = sockets::createSocket(sockets::INET, helpers::integral(type), 0);
    sockets::enableKeepAlive(socket);
    isValid = socket.isValid;
    return fromBool(isValid);
}

OperationResult Socket::Send(Buffer& data, size_t size) const
{
    if (!isValid)
        return OperationResult::Error;
    return sockets::sendData(socket, data, size);
}

OperationResult Socket::SendTo(Buffer &data, size_t size, const std::string& ip, unsigned short port)
{
    if (!isValid)
        return OperationResult::Error;
    return sockets::sendDataTo(socket, data, size, {port, ip, sockets::INET});
}

OperationResult Socket::Recieve(Buffer& buff, size_t size, bool peek) const
{
    if (!isValid)                                         
        return OperationResult::Error;
    return sockets::getData(socket, buff, size, peek);
}

OperationResult Socket::RecieveFrom(Buffer &buff, size_t size, std::string& ip, unsigned short& port, bool peek)
{
    if (!isValid)
        return OperationResult::Error;
    sockets::AddressIn addr;
    OperationResult res = sockets::getDataFrom(socket, buff, size, addr, peek);
    ip = addr.ip;
    port = addr.port;
    return res;
}

sockets::Socket& Socket::getSocket()
{
    return socket;
}

const sockets::Socket& Socket::getSocket() const
{
    return socket;
}

bool Socket::IsValid() const
{
    return isValid;
}

OperationResult Socket::Accept(Socket& client) const
{
    sockets::AddressIn clientAddr;

    if (!isValid || !isBound)
        return OperationResult::Error;
    client = sockets::accept(socket, clientAddr);
    client.ip = clientAddr.ip;
    client.port = clientAddr.port;
    return fromBool(client.isValid);
}

OperationResult Socket::Connect(const std::string& ip, unsigned short port)
{
    return fromBool(sockets::connect(socket, {port, ip, sockets::INET}));
}

std::string Socket::getIp() const
{
    return ip;
}

unsigned short Socket::getPort() const
{
    return port;
}

bool Socket::IsConnected(bool OOB) const
{
    return sockets::isConnectionAlive(socket, OOB);
}

void Socket::SetReadTimeout(int timeout)
{
    sockets::setReadTimeout(socket, timeout);
}

void Socket::SetWriteTimeout(int timeout)
{
    sockets::setWriteTimeout(socket, timeout);
}

bool Socket::operator==(const Socket& rhs)
{
    return (ip == rhs.ip) && (port == rhs.port);
}

SelectResult select(const std::vector<Socket*>& readable, const std::vector<Socket*>& writeable, const std::vector<Socket*>& haveExceptions, int timeout)
{
    fd_set r, w, o;
    bool read = readable.size() != 0;
    bool write = writeable.size() != 0;
    bool oob = haveExceptions.size() != 0;
    timeval time = {0, timeout};
    int maxDescriptorValue = getMaxSocketDescriptorValue(writeable, readable, haveExceptions);

    FD_ZERO(&r);
    FD_ZERO(&w);
    FD_ZERO(&o);
    for (const Socket* sock: readable)
        FD_SET(sock->getSocket().socket, &r);
    for (const Socket* sock: writeable)
        FD_SET(sock->getSocket().socket, &w);
    for (const Socket* sock: haveExceptions)
        FD_SET(sock->getSocket().socket, &o);

    bool isAnyReady = ::select(maxDescriptorValue + 1,
                read ? &r : nullptr,
                write ? &w : nullptr,
                oob ? &o : nullptr,
                &time) > 0;
    return buildSelectResult(isAnyReady, r, w, o, readable, writeable, haveExceptions);
}

int getMaxSocketDescriptorValue(const std::vector<Socket*>& first,
                                const std::vector<Socket*>& second,
                                const std::vector<Socket*>& third)
{
    int maxVal = -1, tmp = -1;

    if ((tmp = helpers::getMaxSocketDescriptor(first)) > maxVal)
        maxVal = tmp;
    if ((tmp = helpers::getMaxSocketDescriptor(second)) > maxVal)
        maxVal = tmp;
    if ((tmp = helpers::getMaxSocketDescriptor(third)) > maxVal)
        maxVal = tmp;
    return maxVal;
}

SelectResult buildSelectResult(bool ready, fd_set& r, fd_set& w, fd_set& e,
                               const std::vector<Socket*>& r_original,
                               const std::vector<Socket*>& w_original,
                               const std::vector<Socket*>& e_original)
{
    SelectResult result;

    result.isAnyReady = ready;
    for (const Socket* sock: r_original)
        if (FD_ISSET(sock->getSocket().socket, &r))
            result.readable.push_back((Socket*)sock);
    for (const Socket* sock: w_original)
        if (FD_ISSET(sock->getSocket().socket, &w))
            result.writeable.push_back((Socket*)sock);
    for (const Socket* sock: e_original)
        if (FD_ISSET(sock->getSocket().socket, &e))
            result.haveExceptions.push_back((Socket*)sock);
    return result;
}

OperationResult fromBool(bool result)
{
    if (result)
        return OperationResult::Success;
    return OperationResult::Error;
}

bool asBool(const OperationResult& result)
{
    if (result == OperationResult::Error || result == OperationResult::ConnectionClosed)
        return false;
    return true;
}

bool isEndpointState(const OperationResult& result)
{
    if (result == OperationResult::PartiallyFinished || result == OperationResult::Timeout)
        return false;
    return true;
}
