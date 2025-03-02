#pragma once
#include "Server.hpp"

class MyServer : public Server<Client<Datacache>>
{
public:
    // chose a name to recognize
    MyServer(std::string name);
    void receive_task(std::shared_ptr<Client<Datacache>> client, uint32_t flagTask) override;

    ~MyServer();
};