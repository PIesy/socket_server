#include "helpers.h"
#include <fstream>
#ifdef __unix__
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef __unix__
extern char **environ;
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
        //    return;
        //while (remainingSize > 0)
        //{
        //    if (remainingSize < zeroBuffSize)
        //    {
        //        f.write(zeroBuff, remainingSize);
        //        remainingSize = 0;
        //    }
        //    else
        //    {
        //        f.write(zeroBuff, zeroBuffSize);
        //        remainingSize -= zeroBuffSize;
        //    }
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
        int f = open64(file.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRGRP | S_IROTH);

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

    ProcessDescripor createProcess(const std::string& executable, std::vector<std::string> args)
    {
        ProcessDescripor desc;
        char** arguments;
        arguments = new char*[args.size() + 1]();
        for (size_t i = 0; i < args.size(); i++)
        {
            arguments[i] = new char[args[i].length() + 1]();
            memcpy(arguments[i], args[i].c_str(), args[i].length());
        }

        posix_spawn(&desc.process, executable.c_str(), nullptr, nullptr, arguments, environ);
        return desc;
    }

    void waitProcess(ProcessDescripor& descriptor)
    {
        int status;

        waitpid(descriptor.process, &status, 0);
    }

    SharedMemoryDescriptor createSharedMemory(size_t size, const std::string& name)
    {
        SharedMemoryDescriptor result;

        result.name = name;
        result.size = size;
        result.handle = shm_open(name.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
        ftruncate(result.handle, size);
        result.memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, result.handle, 0);
        return result;
    }

    SharedMemoryDescriptor openSharedMemory(size_t size, const std::string& name)
    {
        SharedMemoryDescriptor result;

        result.name = name;
        result.size = size;
        result.handle = shm_open(name.c_str(), O_RDWR, 0);
        result.memory = mmap(nullptr, size, PROT_READ, MAP_SHARED, result.handle, 0);
        return result;
    }

    void removeSharedMemory(SharedMemoryDescriptor& desc)
    {
        shm_unlink(desc.name.c_str());
        munmap(desc.memory, desc.size);
    }

#endif

}
