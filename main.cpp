#include <iostream>
#include "server.h"
#include <fstream>

int main(int argc, char** argv)
{
    Server server(3);
    std::string str;
	long port = 2115;
	std::string ip = "127.0.0.1";
	if (argc > 2)
	{
		ip = argv[1];
		port = std::strtol(argv[2], nullptr, 10);
	}
    if (!server.Listen(port, ip))
    {
        std::cerr << "Cannot bind socket" << std::endl;
        return -1;
    }

	
    while (1)
    {
        std::cin >> str;
        if (str == "exit")
        {
            server.Shutdown();
            break;
        }
    }
    return 0;
}
