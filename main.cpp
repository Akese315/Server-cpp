#include "Server.h"
#include "PlayerManager.h"
#include <csignal>

Server server;




void cleanup(int signum)
{
    std::string quit;
    std::cout << "\n";
    Console::printWarning("are you sure you want to leave? (quit)");
    std::cin >> quit;
    if(quit == "quit")
    {
        exit(signum);
    }else
    {
        Console::printInfo("abort failed");
    }        
}

void cleanup()
{
     server.stop();
}

void connectCallback(Protocol::Sx_packet packet, Player * player)
{

}

void stateCallback(Protocol::Sx_packet packet, Player * player)
{

}

void positionCallback(Protocol::Sx_packet packet, Player * player)
{

}

void messageCallback(Protocol::Sx_packet packet, Player * player)
{

}

int main()
{
    unsigned short int port =  3000;    
    PlayerManager *playerManager = new PlayerManager();

    Listener listener = server.createListener(Server::TCP, port,-1);
    Listener listener2 = server.createListener(Server::UDP, port,-1);
    server.addListener(listener);
    server.addListener(listener2);
    server.addClientManager(playerManager);
    server.init();
  
    struct sigaction siga;
    siga.sa_handler = cleanup;                                                                                                                                                                                                                                                                   
    sigaction(SIGINT,&siga,NULL);
    atexit(cleanup);

    server.setEventListener("state", stateCallback);
    server.setEventListener("position", positionCallback);
    server.setEventListener("message", messageCallback);

    server.serverStart();   
   
    return 0;
}

