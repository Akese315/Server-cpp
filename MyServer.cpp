#include "MyServer.hpp"

MyServer::MyServer(std::string name) : Server(name)
{
}

MyServer::~MyServer()
{
}

void MyServer::receive_task(std::shared_ptr<Client<Datacache>> client, uint32_t flagTask)
{
    if (flagTask == MyServer::DATA)
    {
        char tempBuffer[1024];
        int byte = client->receiveData(tempBuffer, 1024);
        tempBuffer[byte] = '\0';
        std::cout << client->get_adress_str() << " said : " << tempBuffer << std::endl;
    }
    if (flagTask == MyServer::CONNECTION)
    {
        Logger::add_logs("new connection : " + std::to_string(client->getSocket()), LogLevel::PASS);
        client->sendData("WELCOME_MESSAGE", 21);
    }
    if (flagTask == MyServer::CLIENT_DISCONNECTION)
    {
        Logger::add_logs(client->get_adress_str() + " has Flags::DISCONNECTED.", LogLevel::WARNING);
    }
    if (flagTask == 2)
    {
        std::cout << "app" << std::endl;
    }
}