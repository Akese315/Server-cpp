#include "Client.h"

Client::Client(uint32_t address, ushort port)
{
    this->address = address;
    this->currentData = new Datacache();
    this->port = port;
}

std::string Client::getIP()
{
    char ip[4];
    struct in_addr addr{};
    addr.s_addr = this->address;
    return inet_ntoa(addr);
}

ushort Client::getPort()
{
    return this->port;
}

Datacache * Client::getCurrentDatacache()
{
    return currentData;
}


void Client::setPreviousData()
{
    if(this->previousData != nullptr)
    {
        delete this->previousData;
    }
    this->previousData = this->currentData;
    this->currentData = new Datacache;
}

