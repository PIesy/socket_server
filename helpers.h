#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include "socket.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __unix__
#include <sys/types.h>
#include <sys/wait.h>
using PROCESS_INFORMATION = pid_t;
using HANDLE = int;
#endif


namespace helpers
{

struct ProcessDescripor
{
    PROCESS_INFORMATION process;
#ifdef _WIN32
    STARTUPINFO si;
#endif
};

struct SharedMemoryDescriptor
{
    HANDLE handle;
    std::string name;
    void* memory;
    size_t size;
};

template<typename T>
constexpr typename std::underlying_type<T>::type integral(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}

void preallocateFile(const std::string& file, size_t size);

int getMaxSocketDescriptor(const std::vector<Socket*>& sockets);

ProcessDescripor createProcess(const std::string& executable, std::vector<std::string> args);

void waitProcess(ProcessDescripor& descriptor);

SharedMemoryDescriptor createSharedMemory(size_t size, const std::string& name);

SharedMemoryDescriptor openSharedMemory(size_t size, const std::string& name);

void removeSharedMemory(SharedMemoryDescriptor& desc);

}

#endif
