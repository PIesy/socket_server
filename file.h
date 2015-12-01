#ifndef FILE_H_
#define FILE_H_

#include <fstream>
#include <string>
#include <unordered_map>
#include "buffer.h"

constexpr size_t FILE_BUFFER_SIZE = 100000000;

class File
{
    std::fstream file;
    size_t bufferedPosition = 0;
    size_t bufferedWriteSize = 0;
    Buffer buff = { FILE_BUFFER_SIZE };
public:
    bool Open(const std::string& string);
    void Close();
    void Write(const void* data, size_t size, size_t position);
    void Flush();

};

#endif FILE_H_
