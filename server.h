#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <string>
#include <vector>
#include <condition_variable>
#include <fstream>
#include <unordered_map>
#include <memory>
#include "data.h"
#include "socket.h"
#include "transfer.h"
#include "tcpclient.h"
#include "udpclient.h"

enum class HandlerType {None, UDP, TCP};

class Server
{
    bool shutdown = false;
    int maxConnectionsPerThread = 100;
    HandlerType requiredHandler = HandlerType::None;
    Socket tcpServerSocket;
    Socket udpServerSocket;
    std::vector<std::thread> threads;
    std::unordered_map<u_int16_t, MachineState> tcpClients;
    std::unordered_map<u_int16_t, ClientContainer> udpClients;
    std::mutex handlerMutex;
    std::condition_variable handlerBusy;
    ClientContainer getUdpClient(u_int8_t id, const std::string& ip, unsigned short port);
    MachineState& getState(u_int8_t id);
    void udpConnectionHandler();
    void tcpConnectionHandler(ClientContainer client);
    OperationResult getData(ClientContainer client);
    OperationResult handleEcho(ClientContainer client);
    OperationResult handleFileTransferInit(ClientContainer client);
    OperationResult handleFileTransfer(ClientContainer client);
    OperationResult handleFileTransferPrepare(ClientContainer client);
    OperationResult handleFileTransferExecute(ClientContainer client);
    OperationResult serveAll(std::list<ClientContainer>& clients);
    OperationResult serve(ClientContainer client);
    void parseMessage(ClientContainer client);
    void parseCommand(ClientContainer client);
    void spawner();
public:
    Server(int connectionsPerThread = 100);
    ~Server();
    bool Listen(unsigned short port, const std::string& ip);
    void Shutdown();
};

#endif // SERVER_H
