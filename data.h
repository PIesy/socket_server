#ifndef DATA_H
#define DATA_H

#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <bitset>

using byte = unsigned char;

#ifdef _WIN32
using u_int8_t = unsigned char;
using u_int16_t = unsigned short;
using u_int32_t = unsigned;
#endif

enum class OperationResult { Timeout, Error, Success, ConnectionClosed, PartiallyFinished, NotConfirmed };
enum class ServerResponce:byte { Success, Error, Retry, Reset };
enum class ServerCommand:byte { Echo, FileTransferStart, FileTransferExecute, Identify };

struct Header
{
    u_int8_t zero = 0;
    u_int8_t id = 0;
    u_int8_t needsConfirmation = 0;
    u_int8_t command = 0;
    u_int32_t packageSize;
};

struct FileInitPackage
{
    Header header;
    u_int32_t chunksCount;
    u_int32_t chunkSize;
    size_t fileSize;
    char fileName[256] = {0};
    FileInitPackage(u_int32_t chunksCount = 0, u_int32_t chunkSize = 0, size_t fileSize = 0)
    {
        this->chunksCount = chunksCount;
        this->chunkSize = chunkSize;
        this->fileSize = fileSize;
    }
};

struct FileTransferPackage
{
    Header header;
    u_int8_t isMarker = 0;
    u_int32_t chunkId = 0;
    u_int32_t size = 0;
    FileTransferPackage(u_int32_t chunkId = 0):chunkId(chunkId) {}
};

struct EchoPackage
{
    Header header;
    u_int32_t stringSize;
    EchoPackage(u_int32_t stringSize = 0):stringSize(stringSize) {}
};

constexpr size_t markerResponceSize = sizeof(std::bitset<512>);

struct MarkerResponce
{
    std::bitset<512> bits;
};

struct ServerResponcePackage
{
    u_int8_t responce;
    ServerResponcePackage(u_int8_t responce = 0):responce(responce) {}
};

#endif // DATA_H

