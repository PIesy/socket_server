#ifdef __unix__
#include "sockportable.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>

namespace sockets {

sockaddr_in buildAddress(const AddressIn& address);
AddressIn fillAddress(const sockaddr_in& addr);
OperationResult parseReturnValue(int value, int expectedValue);
int zeroesToDrop(char* buff, int buffSize);

void init() {}

Socket createSocket(int domain, int type, int protocol)
{
    int sock = socket(domain, type, protocol);
    Socket result;

    result.isValid = sock != -1;
    result.socket = sock;
    return result;
}

bool bindSocket(Socket& socket, const AddressIn& address)
{
    sockaddr_in addr = buildAddress(address);
    int err = bind(socket.socket, (sockaddr*)&addr, sizeof(addr));

    if (err < 0)
        return false;
    return true;
}

bool listen(Socket& socket, int numberOfConnections)
{
    if (::listen(socket.socket, numberOfConnections) < 0)
        return false;
    return true;
}

Socket accept(const Socket& socket, AddressIn& outputAddress)
{
    sockaddr_in output;
    size_t sz = sizeof(output);
    Socket sock;

    memset(&output, 0, sizeof(output));
    int result = ::accept(socket.socket, (sockaddr*)&output, (unsigned*)&sz);
    outputAddress = fillAddress(output);
    sock.isValid = result != -1;
    sock.socket = result;
    sock.address = outputAddress;
    return sock;
}

bool connect(const Socket& socket, const AddressIn& outputAddress)
{
    sockaddr_in output = buildAddress(outputAddress);
    size_t sz = sizeof(output);
    Socket sock;

    int result = ::connect(socket.socket, (sockaddr*)&output, (unsigned)sz);
    return result != -1;
}

sockaddr_in buildAddress(const AddressIn& address)
{
    sockaddr_in addr;
    in_addr p;

    memset(&addr, 0, sizeof(addr));
    p.s_addr = inet_addr(address.ip.c_str());
    addr.sin_family = address.domain;
    addr.sin_port = htons(address.port);
    addr.sin_addr = p;
    return addr;
}

AddressIn fillAddress(const sockaddr_in& addr)
{
    AddressIn result;

    result.domain = addr.sin_family;
    result.ip = inet_ntoa(addr.sin_addr);
    result.port = ntohs(addr.sin_port);
    return result;
}

void closeSocket(const Socket& socket)
{
    shutdown(socket.socket, SHUT_RDWR);
    close(socket.socket);
}

bool isConnectionAlive(const Socket& socket, bool OOB)
{
    int error = 0;
    size_t sz = sizeof(int);
    char buff = 0;
    int flag = OOB ? MSG_OOB : 0;

    errno = 0;
    error = recv(socket.socket, &buff, 1, MSG_PEEK + flag);
    if (error <= 0 && (errno != EWOULDBLOCK && errno != ETIMEDOUT && errno != EINTR))
        return false;

    getsockopt(socket.socket, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&sz);
    return error == 0;
}

void setReadTimeout(const Socket& socket, int timeout)
{
    timeval t;
    t.tv_usec = 0;
    t.tv_sec = timeout;
    setsockopt(socket.socket, SOL_SOCKET, SO_RCVTIMEO, &t, (socklen_t)sizeof(timeval));
}

void setWriteTimeout(const Socket& socket, int timeout)
{
    timeval t;
    t.tv_sec = 0;
    t.tv_usec = timeout;
    setsockopt(socket.socket, SOL_SOCKET, SO_SNDTIMEO, &t, (socklen_t)sizeof(timeval));
}

unsigned getSendBuffSize(const Socket& socket)
{
    unsigned result;
    size_t sz = sizeof(result);

    getsockopt(socket.socket, SOL_SOCKET, SO_SNDBUF, &result, (socklen_t*)&sz);
    return result;
}

void setSendBuffSize(const Socket& socket, unsigned size)
{
    setsockopt(socket.socket, SOL_SOCKET, SO_SNDBUF, &size, (socklen_t)sizeof(size));
}

unsigned getRecieveBuffSize(const Socket& socket)
{
    unsigned result;
    size_t sz = sizeof(result);

    getsockopt(socket.socket, SOL_SOCKET, SO_RCVBUF, &result, (socklen_t*)&sz);
    return result;
}

void setRecieveBuffSize(const Socket& socket, unsigned size)
{
    setsockopt(socket.socket, SOL_SOCKET, SO_RCVBUF, &size, (socklen_t)sizeof(size));
}

void enableKeepAlive(const Socket& socket)
{
    int val = 1;

    setsockopt(socket.socket, SOL_SOCKET, SO_KEEPALIVE, &val, (socklen_t)sizeof(val));
}

OperationResult getData(const Socket& socket, Buffer& buff, size_t size, bool peek)
{
    if (buff.getSize() < size)
        return OperationResult::Error;
    int result = recv(socket.socket, buff.getWritePointer(), size, peek ? MSG_PEEK : 0);

    if (result > 0)
        buff.setWriteOffset(result);
    return parseReturnValue(result, size);
}

OperationResult getDataFrom(const Socket& socket, Buffer& buff, size_t size, AddressIn& addr, bool peek)
{
    if (buff.getSize() < size)
        return OperationResult::Error;
    sockaddr_in ad;
    size_t sz = sizeof(ad);
    memset(&ad, 0, sz);
    int result = recvfrom(socket.socket, buff.getWritePointer(), size, peek ? MSG_PEEK : 0, (sockaddr*)&ad, (socklen_t*)&sz);

    if (result > 0)
    {
        addr = fillAddress(ad);
        buff.setWriteOffset(result);
    }
    return parseReturnValue(result, size);
}

OperationResult sendData(const Socket& socket, Buffer& data, size_t size)
{
    if (data.getSize() < size)
        return OperationResult::Error;
    int result = send(socket.socket, data.getReadPointer(), size, MSG_NOSIGNAL);

    if (result > 0)
        data.setReadOffset(result);
    return parseReturnValue(result, size);
}

OperationResult sendDataTo(const Socket &socket, Buffer &data, size_t size, const AddressIn &addr)
{
    if (data.getSize() < size)
        return OperationResult::Error;
    sockaddr_in ad = buildAddress(addr);
    int result = sendto(socket.socket, data.getReadPointer(), size, 0, (sockaddr*)&ad, sizeof(ad));

    if (result > 0)
        data.setReadOffset(result);
    return parseReturnValue(result, size);
}

OperationResult parseReturnValue(int value, int expectedValue)
{
    if (value >= 0)
    {
        if (value == 0)
            return OperationResult::ConnectionClosed;
        if (value != expectedValue)
            return OperationResult::PartiallyFinished;
        return OperationResult::Success;
    }
    int err = errno;

    if (err == EAGAIN || err == ETIMEDOUT)
        return OperationResult::Timeout;
    std::cerr << "Error: " << errno << std::endl;
    return OperationResult::Error;
}

void end() {}

}

#endif
