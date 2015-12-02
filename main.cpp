#include <iostream>
#include "server.h"
#include "helpers.h"

void handleChildMode(char** argv);
void handleParentMode(int argc, char** argv);

int main(int argc, char** argv)
{
    std::cerr << "Process started" << std::endl;
    if (std::string(argv[0]) == "child")
        handleChildMode(argv);
    else
        handleParentMode(argc, argv);
    std::cerr << "Process terminated" << std::endl;
    return 0;
}

void handleChildMode(char** argv)
{
    std::cerr << "This is child process " << argv[1] << std::endl;
    helpers::SharedMemoryDescriptor desc = helpers::openSharedMemory(sizeof(ChildProcessData), argv[1]);
    ChildProcessData* data = (ChildProcessData*)desc.memory;
    Server server("noexec", true);
    Socket sock(data->socket);
    if (std::string(argv[2]) == "tcp")
        server.TcpConnectionHandler(std::make_shared<TCPClient>(sock));
    else
        server.UdpConnectionHandler(std::move(sock));
    helpers::removeSharedMemory(desc);
}

void handleParentMode(int argc, char **argv)
{
    long port = 2115;
    std::string ip = "127.0.0.1";
    Server server(argv[0]);
    std::string str;

    if (argc > 2)
    {
        ip = argv[1];
        port = std::strtol(argv[2], nullptr, 10);
    }
    if (!server.Listen(port, ip))
    {
        std::cerr << "Cannot bind socket" << std::endl;
        return;
    }
    while (1)
    {
        std::cin >> str;
        if (str == "exit")
        {
            server.Shutdown();
            break;
        }
        if (str == "mp" || str == "enable multiprocessing")
            server.EnableMultiprocessMode();
    }
}
