#ifdef _WIN32
#include "sockportable.h"
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <WS2tcpip.h>

namespace sockets {

sockaddr_in buildAddress(const AddressIn& address);
AddressIn fillAddress(const sockaddr_in& addr);
OperationResult parseReturnValue(int value, int expectedValue);

void init()
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);
}

Socket createSocket(int domain, int type, int protocol)
{
    SOCKET sock = WSASocketW(domain, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
    Socket result;

    result.isValid = sock != -1;
    result.socket = sock;
	result.type = type;
    return result;
}

bool bindSocket(Socket& socket, const AddressIn& address)
{
    sockaddr_in addr = buildAddress(address);

    if (bind(socket.socket, (sockaddr*)&addr, sizeof(addr)) < 0)
        return false;
	socket.address = address;
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
    SOCKET result = ::accept(socket.socket, (sockaddr*)&output, (int*)&sz);
    outputAddress = fillAddress(output);
    sock.isValid = true;
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
    long ip;

    memset(&addr, 0, sizeof(addr));
    inet_pton(address.domain, address.ip.c_str(), &ip);
    p.s_addr = ip;
    addr.sin_family = address.domain;
    addr.sin_port = htons(address.port);
    addr.sin_addr = p;
    return addr;
}

AddressIn fillAddress(const sockaddr_in& addr)
{
    AddressIn result;
    char buff[18] = { 0 };

    result.domain = addr.sin_family;
    result.ip = inet_ntop(AF_INET, (PVOID)&addr.sin_addr, buff, sizeof(buff));
    result.port = ntohs(addr.sin_port);
    return result;
}

void closeSocket(const Socket& socket)
{
    shutdown(socket.socket, SD_BOTH);
    closesocket(socket.socket);
}

bool connect(Socket& socket, AddressIn& outputAddress)
{
    sockaddr_in output = buildAddress(outputAddress);
    size_t sz = sizeof(output);
    Socket sock;

    int result = ::connect(socket.socket, (sockaddr*)&output, (unsigned)sz);
    return result != -1;
}

bool isConnectionAlive(const Socket& socket, bool OOB)
{
    char error = 0;
	char buff = 0;
    size_t sz = sizeof(error);
	int err = 0, result = 0;
	result = recv(socket.socket, &error, 1, MSG_PEEK);
	err = WSAGetLastError();
	if (result == -1 && (err != WSAETIMEDOUT && err != WSAEWOULDBLOCK))
		return false;

    getsockopt(socket.socket, SOL_SOCKET, SO_ERROR, &error, (int*)&sz);
    return error == 0;
}

void setReadTimeout(Socket& socket, int timeout)
{
	setsockopt(socket.socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(DWORD));
}

void setWriteTimeout(Socket& socket, int timeout)
{
	setsockopt(socket.socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(DWORD));
}

unsigned getSendBuffSize(Socket& socket)
{
	unsigned result;
	size_t sz = sizeof(result);

    getsockopt(socket.socket, SOL_SOCKET, SO_SNDBUF, (char*)&result, (int*)&sz);
	return result;
}

void setSendBuffSize(Socket& socket, unsigned size)
{
	setsockopt(socket.socket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, (int)sizeof(size));
}

void enableKeepAlive(Socket& socket)
{
	BOOL val = TRUE;

	setsockopt(socket.socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&val, (int)sizeof(val));
}

OperationResult getData(const Socket& socket, Buffer& buff, size_t size, bool OOB)
{
	if (buff.getSize() < size)
		return OperationResult::Error;
	int result = recv(socket.socket, (char*)buff.getData(), size, OOB ? MSG_OOB : 0);

	if (result > 0)
		buff.setBytesWritten(result);
	return parseReturnValue(result, size);
}

OperationResult getDataFrom(const Socket& socket, Buffer& buff, size_t size, AddressIn& addr, bool peek)
{
    if (buff.getSize() < size)
        return OperationResult::Error;
    sockaddr_in ad;
    size_t sz = sizeof(ad);
    memset(&ad, 0, sz);
    int result = recvfrom(socket.socket, (char*)buff.getWritePointer(), size, peek ? MSG_PEEK : 0, (sockaddr*)&ad, (int*)&sz);

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
    int result = send(socket.socket, (char*)data.getReadPointer(), size, 0);

    if (result > 0)
        data.setReadOffset(result);
    return parseReturnValue(result, size);
}

OperationResult sendDataTo(const Socket &socket, Buffer &data, size_t size, const AddressIn &addr)
{
    if (data.getSize() < size)
        return OperationResult::Error;
    sockaddr_in ad = buildAddress(addr);
    int result = sendto(socket.socket, (char*)data.getReadPointer(), size, 0, (sockaddr*)&ad, sizeof(ad));

    if (result > 0)
        data.setReadOffset(result);
    return parseReturnValue(result, size);
}

OperationResult parseReturnValue(int value, int expectedValue)
{
	if (value > 0)
	{
		if (value != expectedValue)
			return OperationResult::PartiallyFinished;
		return OperationResult::Success;
	}
	if (value == 0)
		return OperationResult::ConnectionClosed;
	int err = WSAGetLastError();

	switch (err)
	{
	case WSAEWOULDBLOCK:
	case WSAETIMEDOUT:
		return OperationResult::Timeout;
	default:
		break;
	}
	return OperationResult::Error;
}

void end()
{
    WSACleanup();
}

}
#endif
