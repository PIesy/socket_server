#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include "socket.h"

namespace helpers
{

template<typename T>
constexpr typename std::underlying_type<T>::type integral(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}

void preallocateFile(const std::string& file, size_t size);

int getMaxSocketDescriptor(const std::vector<Socket*>& sockets);

}

#endif
