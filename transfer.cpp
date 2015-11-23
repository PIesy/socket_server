#include "transfer.h"
#include "helpers.h"
#include <iostream>
#include <thread>
#include <chrono>

Transfer::Transfer() {}

Transfer::Transfer(const TransferFunction& recieve, const TransferFunction& send)
{
    this->recieve = recieve;
    this->send = send;
}

Transfer::~Transfer()
{
    End();
}

void Transfer::setRecieveFunction(const TransferFunction& recieve)
{
    this->recieve = recieve;
}

void Transfer::setSendFunction(const TransferFunction& send)
{
    this->send = send;
}

OperationResult Transfer::Start(TransferType type, size_t size, bool needsConfirmation)
{
    if (transferFinished)
    {
        this->type = type;
        transferFinished = false;
        confirmed = !needsConfirmation;
        transferSize = size;
        return OperationResult::Success;
    }
    return OperationResult::Error;
}

OperationResult Transfer::Start(TransferType type, size_t size, const ConfirmCondition& condition)
{
    if (transferFinished && confirmed)
    {
        this->type = type;
        transferFinished = false;
        confirmed = false;
        transferSize = size;
        confirmCondition = condition;
        return OperationResult::Success;
    }
    return OperationResult::Error;
}

OperationResult Transfer::Execute(Buffer& buff)
{
    if (transferFinished && confirmed)
        return OperationResult::Success;
    OperationResult result = OperationResult::Error;

    if (!transferFinished)
    {
        switch (type)
        {
        case TransferType::None:
            result = OperationResult::Success;
            break;
        case TransferType::Send:
            result = send(buff, transferSize - buff.getReadOffset());
            break;
        case TransferType::Recieve:
            result = recieve(buff, transferSize - buff.getWriteOffset());
            break;
        }
    }
    if (result == OperationResult::Success && !transferFinished)
        transferFinished = true;
    if (confirmCondition && transferFinished && !confirmed)
        confirmed = !confirmCondition(buff);
    if (transferFinished && !confirmed)
    {
        if ((result = confirm()) == OperationResult::Success)
            confirmed = true;
        else
        {
            std::cerr << "Confirmation error" << std::endl;
            End();
            buff.Clear();
        }
    }
    return result;
}

OperationResult Transfer::confirm()
{
    ServerResponcePackage responce(helpers::integral(ServerResponce::Success));
    Buffer buff(&responce);
    if (type == TransferType::Recieve)
        return send(buff, 1);
    //std::cerr << "Waiting for confirmation" << std::endl;
    if (recieve(buff, 1) != OperationResult::Success)
        return OperationResult::Error;
    return OperationResult::Success;
}

void Transfer::End(bool resetClient)
{
    if (resetClient)
    {
        ServerResponcePackage responce(helpers::integral(ServerResponce::Reset));
        Buffer buff(&responce);
        if (!transferFinished)
            send(buff, 1);
    }
    transferFinished = true;
    confirmed = true;
}
