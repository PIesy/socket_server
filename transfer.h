#ifndef TRANSFER_H
#define TRANSFER_H

#include "buffer.h"
#include "socket.h"
#include "functional"

enum class TransferType {Recieve, Send, None};

using TransferFunction = std::function<OperationResult(Buffer&, size_t)>;
using ConfirmCondition = std::function<bool(Buffer&)>;

class Transfer
{
    TransferType type = TransferType::None;
    bool transferFinished = true;
    bool confirmed = true;
    size_t transferSize;
    TransferFunction recieve, send;
    ConfirmCondition confirmCondition;
    OperationResult confirm();
public:
    Transfer();
    Transfer(const TransferFunction& recieve, const TransferFunction& send);
    ~Transfer();
    void setRecieveFunction(const TransferFunction& recieve);
    void setSendFunction(const TransferFunction& send);
    OperationResult Start(TransferType type, size_t size, bool needsConfirmation = false);
    OperationResult Start(TransferType type, size_t size, const ConfirmCondition& condition);
    OperationResult Execute(Buffer& buff);
    void End(bool resetClient = false);
};

#endif // TRANSFER_H
