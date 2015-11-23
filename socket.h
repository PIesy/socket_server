#ifndef SOCKET_H
#define SOCKET_H

#include "sockportable.h"
#include "data.h"
#include "buffer.h"
#include <string>
#include <vector>

enum class SocketType {TCP = sockets::STREAM, UDP = sockets::DATAGRAM};

class Socket;

struct SelectResult
{
    bool isAnyReady = false;
    std::vector<Socket*> readable;
    std::vector<Socket*> writeable;
    std::vector<Socket*> haveExceptions;

    operator bool() { return isAnyReady; }
};

class Socket
{
    sockets::Socket socket;
    bool isValid = false;
    bool isBound = false;
    std::string ip = "";
    unsigned short port = 0;
public:
    Socket();
    Socket(const sockets::Socket& rhs);
    Socket(const Socket& rhs) = delete;
    Socket(Socket&& rhs);
    Socket(SocketType type, const std::string& ip = "", unsigned short port = 0);
    ~Socket();
    bool IsValid() const;
    OperationResult Create(SocketType type);
    OperationResult Bind(const std::string& ip, unsigned short port);
    OperationResult Send(Buffer& data, size_t size) const;
    OperationResult SendTo(Buffer& data, size_t size, const std::string& ip, unsigned short port);
    OperationResult Recieve(Buffer& buff, size_t size, bool peek = false) const;
    OperationResult RecieveFrom(Buffer& buff, size_t size, std::string& ip, unsigned short& port, bool peek = false);
    OperationResult Listen(int maxClientsNumber = 1024);
    OperationResult Accept(Socket& client) const;
    OperationResult Connect(const std::string& ip, unsigned short port);
    void SetReadTimeout(int timeout);
    void SetWriteTimeout(int timeout);
    bool IsConnected(bool OOB) const;
    sockets::Socket& getSocket();
    const sockets::Socket& getSocket() const;
    std::string getIp() const;
    unsigned short getPort() const;
    Socket& operator=(const Socket& rhs) = delete;
    Socket& operator=(Socket&& rhs);
    Socket& operator=(const sockets::Socket& rhs);
    bool operator==(const Socket& rhs);
};

OperationResult fromBool(bool result);
bool asBool(const OperationResult& result);
bool isEndpointState(const OperationResult& result);
SelectResult select(const std::vector<Socket*>& readable, const std::vector<Socket*>& writeable, const std::vector<Socket*>& haveExceptions, int timeout);

#endif // SOCKET_H
