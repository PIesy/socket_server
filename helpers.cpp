#include "helpers.h"
#include <fstream>
#ifdef __unix__
#include <fcntl.h>
#include <unistd.h>
#endif

namespace helpers
{

#ifdef _WIN32
	void preallocateFile(const std::string & file, size_t size)
	{
		//std::fstream f;
		//char zeroBuff[1024 * 10] = { 0 };
		//size_t zeroBuffSize = sizeof(zeroBuff);
		//size_t remainingSize = size;

  //      f.open(file, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		//if (!f.is_open())
		//	return;
		//while (remainingSize > 0)
		//{
		//	if (remainingSize < zeroBuffSize)
		//	{
		//		f.write(zeroBuff, remainingSize);
		//		remainingSize = 0;
		//	}
		//	else
		//	{
		//		f.write(zeroBuff, zeroBuffSize);
		//		remainingSize -= zeroBuffSize;
		//	}
		//}
		//            No actual speedup this way
	}

    int getMaxSocketDescriptor(const std::vector<Socket*>& sockets)
    {
        return 0;
    }
#endif
#ifdef __unix__
    void preallocateFile(const std::string & file, size_t size)
    {
        int f = open64(file.c_str(), O_CREAT | O_TRUNC | O_RDWR);

        fallocate64(f, 0, 0, size);
        close(f);
    }

    int getMaxSocketDescriptor(const std::vector<Socket*>& sockets)
    {
        int maxVal = -1;

        for (const Socket* sock: sockets)
            if (sock->getSocket().socket > maxVal)
                maxVal = sock->getSocket().socket;
        return maxVal;
    }

#endif

}
