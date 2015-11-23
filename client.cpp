#include "client.h"
#include <algorithm>

ClientStates buildClientStates(const SelectResult& result,  ClientsList writeable,  ClientsList readable,  ClientsList haveExceptions);

Client::Client()
{
    state = new MachineState();
    state->echoMode = true;
}

Client::~Client()
{
    if (isStateDeletable)
        delete state;
}

void Client::setState(MachineState& state)
{
    if (isStateDeletable)
        delete this->state;
    isStateDeletable = false;
    this->state = &state;
}

MachineState& Client::getState()
{
    return *state;
}

ClientStates select(ClientsList writeable,  ClientsList readable,  ClientsList haveExceptions, int timeout)
{
    std::vector<Socket*> r, w, e;

    for (const ClientContainer& cont: writeable)
        w.push_back(&cont->getSocket());
    for (const ClientContainer& cont: readable)
        r.push_back(&cont->getSocket());
    for (const ClientContainer& cont: haveExceptions)
        e.push_back(&cont->getSocket());
    return buildClientStates(select(r, w, e, timeout), writeable, readable, haveExceptions);
}

ClientStates buildClientStates(const SelectResult& result,  ClientsList writeable,  ClientsList readable,  ClientsList haveExceptions)
{
    ClientStates res;
    std::vector<ClientContainer> tmp;

    res.isAnyReady = result.isAnyReady;
    for (const ClientContainer& cont: writeable)
        if (std::find(result.writeable.begin(), result.writeable.end(), &cont->getSocket()) != result.writeable.end())
            tmp.push_back(cont);
    res.clients.insert({ClientStates::Writeable, tmp});
    tmp.clear();

    for (const ClientContainer& cont: readable)
        if (std::find(result.readable.begin(), result.readable.end(), &cont->getSocket()) != result.readable.end())
            tmp.push_back(cont);
    res.clients.insert({ClientStates::Readable, tmp});
    tmp.clear();

    for (const ClientContainer& cont: haveExceptions)
        if (std::find(result.haveExceptions.begin(), result.haveExceptions.end(), &cont->getSocket()) != result.haveExceptions.end())
            tmp.push_back(cont);
    res.clients.insert({ClientStates::HaveExceptions, tmp});
    tmp.clear();

    return res;
}
