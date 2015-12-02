#include "file.h"
#include <ios>
#include <iostream>

bool File::Open(const std::string & string)
{
    file.open(string.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    return file.is_open();
}

void File::Close()
{
    Flush();
    file.close();
}

void File::Write(const void* data, size_t size, size_t position)
{
    if ((bufferedPosition + bufferedWriteSize) != position || size > buff.GetRemainingFreeSpace())
    {
        //std::cerr << "Writing data" << std::endl;
        file.write((const char*)buff.getData(), bufferedWriteSize);
        bufferedWriteSize = 0;
        bufferedPosition = position;
        file.seekp(position, file.beg);
        buff.Clear();
    }
    bufferedWriteSize += size;
    buff.Write(data, size);
}

void File::Flush()
{
    file.write((char*)buff.getData(), bufferedWriteSize);
}
