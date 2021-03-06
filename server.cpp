#include "server.h"
#include <iostream>
#include <cstring>
#include <ios>
#include <list>
#include "helpers.h"

constexpr int MAX_UDP_PACKAGE_SIZE = 1420;

Server::Server(const std::string& executablePath, bool noInit)
{
    sockets::init();
    this->executablePath = executablePath;
    if (noInit)
        return;
    if (tcpServerSocket.Create(SocketType::TCP) == OperationResult::Error)
        std::cerr << "Error in opening tcp socket" << std::endl;
    if (udpServerSocket.Create(SocketType::UDP) == OperationResult::Error)
        std::cerr << "Error in opening udp socket" << std::endl;
    sockets::setRecieveBuffSize(udpServerSocket.getSocket(), 2000000);
    std::cerr << "Buff size: " << sockets::getRecieveBuffSize(udpServerSocket.getSocket()) << std::endl;
    tcpServerSocket.SetWriteTimeout(1);
    udpServerSocket.SetWriteTimeout(1);
}

Server::~Server()
{
    shutdown = true;
    for (std::thread& thread : threads)
    {
        try {
            thread.join();
        }
        catch (std::exception e) {}
    }
    for (helpers::ProcessDescripor process : processes)
        helpers::waitProcess(process);
    for (helpers::SharedMemoryDescriptor mem : sharedData)
        helpers::removeSharedMemory(mem);
    sockets::end();
}

bool Server::Listen(unsigned short port, const std::string& ip)
{
    this->port = port;
    this->ip = ip;
    if (tcpServerSocket.Bind(ip, port) == OperationResult::Error)
        return false;
    if (udpServerSocket.Bind(ip, port) == OperationResult::Error)
        return false;
    if (tcpServerSocket.Listen() == OperationResult::Error)
        return false;
    threads.emplace_back(&Server::udpMainLoop, this);
    threads.emplace_back(&Server::spawner, this);
    return true;
}

void Server::Shutdown()
{
    shutdown = true;
    for (std::thread& thread : threads)
    {
        try
        {
            thread.join();
        }
        catch (std::exception e) {}
    }
}

void Server::UdpConnectionHandler(Socket socket)
{
    std::list<ClientContainer> clients;
    unsigned short port;
    std::string ip;
    Buffer buff(sizeof(Header));

    while (!shutdown)
        if (select({&socket}, {}, {}, 1000000) && !shutdown)
            if (asBool(socket.RecieveFrom(buff, sizeof(Header), ip, port, true)))
            {
                Header& header = buff;
                parseCommand(getUdpClient(header.id, ip, port, socket));
                buff.Clear();
            }
}

ClientContainer Server::getUdpClient(u_int8_t id, const std::string& ip, unsigned short port, Socket& socket)
{
    if (udpClients.count(id))
    {
         ClientContainer& cont = udpClients.at(id);
         ((UDPClient*)(cont.operator ->()))->setIp(ip);
         ((UDPClient*)(cont.operator ->()))->setPort(port);
         return cont;
    }
    std::cerr << "New client id:" << (int)id << " address: " << ip << ":" << port << std::endl;
    udpClients.insert({id, std::make_shared<UDPClient>(socket, ip, port)});
    return udpClients.at(id);
}

void Server::TcpConnectionHandler(ClientContainer client)
{
    std::list<ClientContainer> clients = {client};

    std::cerr << "Ready for connections" << std::endl;
    while (!shutdown && !clients.empty())
        serveAll(clients);
    std::cerr << "Connection handler stopped" << std::endl;
}

void Server::EnableMultiprocessMode()
{
    multiprocessMode = true;
}

OperationResult Server::serveAll(std::list<ClientContainer>& clients)
{
    ClientStates state;

    if ((state = select({}, {clients}, {}, 100000)))
        for (ClientContainer& client: state.clients[state.Readable])
            if (serve(client) == OperationResult::ConnectionClosed)
                clients.remove(client);
    if (clients.empty())
        return OperationResult::ConnectionClosed;
    return OperationResult::Success;
}

OperationResult Server::serve(ClientContainer client)
{
    if (!client->isReachable())
        return OperationResult::ConnectionClosed;
    if (!client->getState().echoMode)
        parseCommand(client);
    else
        parseMessage(client);
    return OperationResult::Success;
}

void Server::parseCommand(ClientContainer client)
{
    using helpers::integral;
    MachineState& state = client->getState();

    if (getData(client) != OperationResult::Success)
        return;
    Header& header = state.buff;
    OperationResult operationCompleted = OperationResult::PartiallyFinished;

    switch (header.command)
    {
    case integral(ServerCommand::FileTransferStart):
        operationCompleted = handleFileTransferInit(client);
        break;
    case integral(ServerCommand::FileTransferExecute):
        operationCompleted = handleFileTransfer(client);
        break;
    case integral(ServerCommand::Echo):
        operationCompleted = handleEcho(client);
        break;
    case integral(ServerCommand::Identify):
        client->setState(getState(header.id));
    }
    if (operationCompleted == OperationResult::Error)
        std::cerr << "Error while executing: " << header.command << std::endl;
    state.buff.Clear();
}

MachineState& Server::getState(u_int8_t id)
{
    if (tcpClients.count(id))
    {
        std::cerr << "Identified client, id: " << (int)id << std::endl;
        return tcpClients.at(id);
    }
    tcpClients.insert({id, {}});
    return tcpClients.at(id);
}

OperationResult Server::getData(ClientContainer client)
{
    Buffer buff(sizeof(Header));
    Header& header = buff;

    if (client->Peek(sizeof(Header), buff) != OperationResult::Success)
        return OperationResult::Error;
    OperationResult result;
    do 
        result = client->Recieve(header.packageSize);
    while (result == OperationResult::PartiallyFinished);
    return  result;
}

OperationResult Server::handleEcho(ClientContainer client)
{
    client->getState().buff.Write("\0", 1);
    client->getState().buff.setReadOffset(sizeof(Header));
    std::string echo((char*)client->getState().buff.getReadPointer());
    std::cerr << "Got string: " << echo << std::endl;
    std::string responce = "<server>: " + echo + "\n";
    Buffer buff(responce.c_str(), responce.length());
    client->Send(responce.length(), buff, false);
    return OperationResult::Success;
}

OperationResult Server::handleFileTransferInit(ClientContainer client)
{
    FileInitPackage& pack = client->getState().buff;
    MachineState& state = client->getState();

    memset(&client->getState().fileTransferState, 0, sizeof(FileTransferState));
    state.fileInitState.chunksCount = pack.chunksCount;
    state.fileInitState.chunksSize = pack.chunkSize;
    state.fileInitState.fileName = pack.fileName;
    state.fileInitState.file.Close();
    std::string fileName = std::to_string(pack.header.id) + "_" + pack.fileName;
    helpers::preallocateFile(fileName, pack.fileSize);
    if (!state.fileInitState.file.Open(fileName))
        std::cerr << "W T F" << std::endl;
    std::cerr << "Recieving file " << pack.fileName << " size: " << pack.fileSize << " chunks: " << pack.chunksCount << std::endl;
    return OperationResult::Success;
}

OperationResult Server::handleFileTransfer(ClientContainer client)
{
    if (handleFileTransferPrepare(client) == OperationResult::Error)
        return OperationResult::Success;
    handleFileTransferExecute(client);
    if (client->getState().fileTransferState.chunksTransfered == client->getState().fileInitState.chunksCount)
    {
        client->getState().fileInitState.file.Close();
        std::cerr << "File download successful" << std::endl;
        return OperationResult::Success;
    }
    return OperationResult::PartiallyFinished;
}

OperationResult Server::handleFileTransferPrepare(ClientContainer client)
{
    if (!client->getState().fileInitState.chunksCount)
        return OperationResult::Error;
    FileTransferState& state = client->getState().fileTransferState;
    FileTransferPackage& pack = client->getState().buff;

    if (pack.isMarker == 1)
    {
        //std::cerr << "Got marker " << pack.size << std::endl;
        respondToMarker(client, pack);
        return OperationResult::Error;
    }
    state.batchState[pack.chunkId % state.batchSize] = true;
    state.nextChunkSize = pack.size;
    state.chunkId = pack.chunkId;
    //std::cerr << " << PACKAGE " << state.chunkId << std::endl;
    client->getState().buff.setReadOffset(sizeof(FileTransferPackage));
    return OperationResult::Success;
}

void Server::respondToMarker(ClientContainer client, FileTransferPackage& pack)
{
    Buffer buff(markerResponceSize);
    MarkerResponce responce;
    responce.bits = client->getState().fileTransferState.batchState;
    responce.chunkId = pack.chunkId - pack.size;
    buff.Write(responce);
    if (pack.size)
    {
        printMissingPackages(client, pack);
        client->getState().fileTransferState.batchSize = pack.size;
        client->Send(buff.getSize(), buff, false);
    }
    if (!pack.size)
        client->getState().fileTransferState.batchState.reset();
}

void Server::printMissingPackages(ClientContainer client, FileTransferPackage& pack)
{
    bool err = false;
    if (pack.chunkId == 0)
        return;
    for (unsigned chunkId = pack.chunkId - pack.size; chunkId < pack.chunkId; chunkId ++)
        if (client->getState().fileTransferState.batchState[chunkId % pack.size] == false)
        {
            err = true;
            std::cerr << "Missing package " << chunkId << std::endl;
        }
    if (err)
        std::cerr << client->getState().fileTransferState.batchState.to_string() << std::endl;
}

OperationResult Server::handleFileTransferExecute(ClientContainer client)
{
    FileInitState& params = client->getState().fileInitState;
    FileTransferState& transferState = client->getState().fileTransferState;
    File& file = params.file;

    file.Write(client->getState().buff.getReadPointer(), transferState.nextChunkSize, transferState.chunkId * params.chunksSize);
    transferState.chunksTransfered++;
    transferState.downladProgress = printDownloadState(transferState.chunksTransfered, params.chunksCount, transferState.downladProgress);
    client->getState().buff.Clear();
    transferState.previousChunkId = transferState.chunkId;
    return OperationResult::Success;
}

double Server::printDownloadState(unsigned chunksTransfered, unsigned chunksCount, double lastValue, double minStep)
{
    double downladProgress = (chunksTransfered / (double)chunksCount) * 100.0;

    if ((downladProgress - lastValue) > minStep)
    {
        std::cerr << "Downloaded " << downladProgress << "%" << std::endl;
        return downladProgress;
    }
    return lastValue;
}

void Server::parseMessage(ClientContainer client)
{
    char tmp[1001] = {0};
    std::string message;

    client->Recieve(100000, false);
    client->endTransfer();
    client->getState().message << (char*)client->getState().buff.getData();
    client->getState().buff.Clear(true);
    while (!client->getState().message.eof())
    {
        memset(tmp, 0, 1000);
        client->getState().message.getline(tmp, 1000, '\n');
        message = tmp;
        if (client->getState().message.eof())
        {
            client->getState().message.clear();
            return;
        }
        if (message == "!lag\r")
            std::this_thread::sleep_for(std::chrono::seconds(5));
        if (message == "!cmd\r")
        {
            std::cerr << "Switched operation mode" << std::endl;
            client->getState().echoMode = false;
        }
        std::cerr << "Got message: " << message << std::endl;
        std::string responce = "<server>: " + message + '\n';
        Buffer buff(responce.c_str(), responce.length());
        client->Send(responce.length(), buff, false);
    }
}

void Server::spawner()
{
    Socket connectionSocket;

    while (!shutdown)
        if (select({&tcpServerSocket}, {}, {}, 1000000) && !shutdown)
        {
            tcpServerSocket.Accept(connectionSocket);
            if (connectionSocket.IsValid())
            {
                std::cerr << "Accepted connection " << connectionSocket.getIp() << ":" << connectionSocket.getPort() << std::endl;
                if (multiprocessMode)
                    launchNewProcess(connectionSocket, SocketType::TCP);
                else
                    threads.emplace_back(&Server::TcpConnectionHandler, this, std::make_shared<TCPClient>(connectionSocket));
            }
        }
    std::cerr << "spawner stopped" << std::endl;
}

void Server::udpMainLoop()
{
    unsigned short port;
    std::string ip;
    Buffer buff(sizeof(Header));
    RedirectResponce redirect;

    std::cerr << "Ready for udp communication" << std::endl;
    while (!shutdown)
        if (select({&udpServerSocket}, {}, {}, 1000000) && !shutdown)
            if (asBool(udpServerSocket.RecieveFrom(buff, sizeof(Header), ip, port)))
            {
                Header& header = buff;
                if (header.zero != 0)
                    parseMessage(std::make_shared<UDPClient>(udpServerSocket, ip, port));
                else
                {
                    Socket clientSocket;
                    clientSocket.Create(SocketType::UDP);
                    clientSocket.Bind(this->ip, this->port + childMemoryId);
                    sockets::setRecieveBuffSize(clientSocket.getSocket(), 2000000);
                    redirect.newPort = this->port + childMemoryId;
                    memcpy(redirect.newIp, this->ip.c_str(), this->ip.length());
                    Buffer msg(sizeof(RedirectResponce));
                    msg.Write(redirect);
                    udpServerSocket.SendTo(msg, msg.getSize(), ip, port);
                    if (multiprocessMode)
                        launchNewProcess(clientSocket, SocketType::UDP);
                    else
                        threads.emplace_back(&Server::UdpConnectionHandler, this, std::move(clientSocket));
                }
                buff.Clear();
            }
}



void Server::launchNewProcess(Socket& socket, SocketType type)
{
    ChildProcessData data;
    helpers::SharedMemoryDescriptor desc = helpers::createSharedMemory(sizeof(data), "child" + std::to_string(childMemoryId));
    std::string typeString = type == SocketType::TCP ? "tcp" : "udp";

    sharedData.push_back(desc);
    data.shutdown = &shutdown;
    data.socket = socket.getSocket();
    socket.Invalidate();
    childMemoryId++;
    memcpy(desc.memory, &data, sizeof(data));
    processes.push_back(helpers::createProcess(executablePath, {"child", desc.name, typeString}));
}
