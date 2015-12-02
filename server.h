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
#include "helpers.h"

struct ChildProcessData
{
    sockets::Socket socket;
    bool* shutdown;
};

class Server
{
    bool shutdown = false;
    bool multiprocessMode = false;
    int childMemoryId = 1;
    std::string executablePath;
    Socket tcpServerSocket;
    Socket udpServerSocket;
    std::string ip;
    unsigned short port;
    std::vector<helpers::SharedMemoryDescriptor> sharedData;
    std::vector<helpers::ProcessDescripor> processes;
    std::vector<std::thread> threads;
    std::unordered_map<u_int16_t, MachineState> tcpClients;
    std::unordered_map<u_int16_t, ClientContainer> udpClients;
    ClientContainer getUdpClient(u_int8_t id, const std::string& ip, unsigned short port, Socket& socket);
    MachineState& getState(u_int8_t id);
    double printDownloadState(unsigned chunksTransfered, unsigned chunksCount, double lastValue, double minStep = 1.0);
    void launchNewProcess(Socket& socket, SocketType type);
    OperationResult getData(ClientContainer client);
    OperationResult handleEcho(ClientContainer client);
    OperationResult handleFileTransferInit(ClientContainer client);
    OperationResult handleFileTransfer(ClientContainer client);
    OperationResult handleFileTransferPrepare(ClientContainer client);
    OperationResult handleFileTransferExecute(ClientContainer client);
    OperationResult serveAll(std::list<ClientContainer>& clients);
    OperationResult serve(ClientContainer client);
    void respondToMarker(ClientContainer client, FileTransferPackage& pack);
    void printMissingPackages(ClientContainer client, FileTransferPackage& pack);
    void parseMessage(ClientContainer client);
    void parseCommand(ClientContainer client);
    void spawner();
    void udpMainLoop();
public:
    Server(const std::string& executablePath, bool noInit = false);
    ~Server();
    bool Listen(unsigned short port, const std::string& ip);
    void UdpConnectionHandler(Socket socket);
    void TcpConnectionHandler(ClientContainer client);
    void EnableMultiprocessMode();
    void Shutdown();
};

#endif // SERVER_H
