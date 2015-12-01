#include "helpers.h"
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif
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
        //HANDLE f = CreateFileA(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        //
        //SetFilePointer(f, size, nullptr, FILE_BEGIN);
        //SetEndOfFile(f);
        //CloseHandle(f);
        std::string cmdString = "fsutil file createnew " + file + " " + std::to_string(size);
        system(cmdString.c_str());
    }

    int getMaxSocketDescriptor(const std::vector<Socket*>& sockets)
    {
        return 0;
    }

    ProcessDescripor createProcess(const std::string& executable, std::vector<std::string> args)
    {
        ProcessDescripor desc;
        std::string arguments;
        memset(&desc.process, 0, sizeof(desc.process));
        memset(&desc.si, 0, sizeof(desc.si));
        char buff[30] = { 0 };
        
        desc.si.cb = sizeof(desc.si);
        for (std::string& arg : args)
            arguments.append(arg + " ");
        memcpy(buff, arguments.c_str(), arguments.length());
        CreateProcessA(executable.c_str(), buff, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &desc.si, &desc.process);
        return desc;
    }

    void waitProcess(ProcessDescripor& descriptor)
    {
        WaitForSingleObject(descriptor.process.hProcess, INFINITE);
        CloseHandle(descriptor.process.hProcess);
        CloseHandle(descriptor.process.hThread);
    }

    SharedMemoryDescriptor createSharedMemory(size_t size, const std::string& name)
    {
        SharedMemoryDescriptor result;

        result.name = name;
        result.size = size;
        result.handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name.c_str());
        result.memory = MapViewOfFile(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
        return result;
    }

    SharedMemoryDescriptor openSharedMemory(size_t size, const std::string& name)
    {
        SharedMemoryDescriptor result;

        result.name = name;
        result.size = size;
        result.handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        result.memory = MapViewOfFile(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
        return result;
    }

    void removeSharedMemory(SharedMemoryDescriptor& desc)
    {
        UnmapViewOfFile(desc.memory);
        CloseHandle(desc.handle);
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
