#ifndef CLIENT_H
#define CLIENT_H

#include <sstream>
#include <unordered_map>
#include <memory>
#include <list>
#include <fstream>
#include "data.h"
#include "buffer.h"
#include "socket.h"
#include "transfer.h"

constexpr size_t MAX_BUFFER_SIZE = 2500000;

class Client;

using ClientContainer = std::shared_ptr<Client>;
using ClientsList = const std::list<ClientContainer>&;

const ConfirmCondition genericConfirmCondition = [](Buffer& buff)
{
    if (buff.getSize() > 2)
        return buff.getData()[2] == 1;
    return false;
};

struct ClientStates
{
    enum State {Readable, Writeable, HaveExceptions};
    std::unordered_map<int, std::vector<ClientContainer>> clients;
    bool isAnyReady = false;
    operator bool() { return isAnyReady; }
};

struct FileInitState
{
    std::fstream file;
    unsigned chunksSize = 0;
    unsigned chunksCount = 0;
    std::string fileName = "";
};

struct FileTransferState
{
    std::bitset<512> batchState = 0;
    unsigned batchSize = 0;
    unsigned chunksTransfered = 0;
    unsigned previousChunkId = -1;
    unsigned chunkId = 0;
    unsigned nextChunkSize = 0;
    double downladProgress = 0;
    double printedProgress = 0;
};

struct MachineState
{
    bool echoMode = false;
    Buffer buff = {MAX_BUFFER_SIZE};
    std::stringstream message;
    FileInitState fileInitState;
    FileTransferState fileTransferState;
    MachineState() {}
    MachineState& operator=(const MachineState&) { return *this; }
    MachineState(const MachineState&) {}
};

class Client
{
protected:
    MachineState* state;
    bool isStateDeletable = true;
public:
    Client();
    virtual ~Client();
    void setState(MachineState& state);
    MachineState& getState();
    virtual OperationResult Send(size_t size, Buffer& buff, bool confirm = true) = 0;
    virtual OperationResult Send(size_t size, bool confirm = true) = 0;
    virtual OperationResult Recieve(size_t size, Buffer& buff, bool confirm = true) = 0;
    virtual OperationResult Recieve(size_t size, bool confirm = true) = 0;
    virtual OperationResult Peek(size_t size, Buffer& buff) = 0;
    virtual OperationResult Peek(size_t size) = 0;
    virtual Socket& getSocket() = 0;
    virtual bool isReachable() = 0;
    virtual void endTransfer() = 0;
};

ClientStates select( ClientsList writeable,  ClientsList readable,  ClientsList haveExceptions, int timeout);

#endif // CLIENT_H
