#ifndef SOCKPORTABLE_H
#define SOCKPORTABLE_H

#include "data.h"
#include <string>
#include "buffer.h"

#ifdef __unix__
#include <arpa/inet.h>
typedef int SOCKET;
#endif
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace sockets {

    enum SelectOperations { READ = 1, WRITE = 2, OOB = 4 };
    enum SocketDomains {INET = AF_INET};
    enum SocketTypes {STREAM = SOCK_STREAM, DATAGRAM = SOCK_DGRAM};

    struct AddressIn
    {
        unsigned short port;
        std::string ip;
        int domain;
    };

    struct Socket
    {
        SOCKET socket;
        bool isValid = false;
		AddressIn address;
		int type = STREAM;
    };

    void init();

    void setReadTimeout(const Socket& socket, int timeout);

    void setWriteTimeout(const Socket& socket, int timeout);

    unsigned getSendBuffSize(const Socket& socket);

    void setSendBuffSize(const Socket& socket, unsigned size);

    unsigned getRecieveBuffSize(const Socket& socket);

    void setRecieveBuffSize(const Socket& socket, unsigned size);

    void enableKeepAlive(const Socket& socket);

    Socket createSocket(int domain, int type, int protocol);

    bool bindSocket(Socket& socket, const AddressIn& address);

    bool listen(Socket& socket, int numberOfConnections);

    Socket accept(const Socket& socket, AddressIn& outputAddress);

    bool connect(const Socket& socket, const AddressIn& outputAddress);

    void closeSocket(const Socket& socket);

    OperationResult getData(const Socket& socket, Buffer& buff, size_t size, bool peek);

    OperationResult getDataFrom(const Socket& socket, Buffer& buff, size_t size, AddressIn& addr, bool peek = false);

    OperationResult sendData(const Socket& socket, Buffer& data, size_t size);

    OperationResult sendDataTo(const Socket& socket, Buffer& data, size_t size, const AddressIn& addr);

    bool isConnectionAlive(const Socket& socket, bool OOB = false);

    void end();

}

#endif // SOCKPORTABLE_H

