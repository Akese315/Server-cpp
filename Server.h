#pragma once
#include <vector>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bitset>
#include <sys/types.h>
#include "Console.h"
#include "PlayerManager.h"
#include "EventData.h"
#include "EventManager.h"
#include "Protocol.h"
#include "ThreadPool.h"

typedef struct Listener
{
    int socket;
    unsigned short type;
    int timeout;
    unsigned short port;
};



class Server
{

private:
    std::vector<Listener> ListenerArray;

    const static unsigned short int MAX_FDS_SIZE = 300;
    const static unsigned short int MAX_PACKET_SIZE = 1024;        
    const char WELCOME_MESSAGE[20] = "Welcome to our game";

    const static ushort TCP_SOCKET = 3;
    const static ushort UDP_SOCKET = 2;
    const static ushort TCP_LISTENER = 1;

    struct pollfd fds[300];
    int activeFds = 0;
    PlayerManager * playerManager;
    EventManager eventManager;
    ThreadPool threadPool;
    
    bool canStart = true;

    

    void getError(const char* where);
    void Disconnected(int *fd);
    void purgeFDs();    
    void addFD(int fd);
    void checkFDStatus(int fdr);
    void Send(Player * player, int fd, ushort type, const char* buffer);
    void Receive(Player * player, int *fd, ushort type);
    bool getContentFast(int *fd, ushort type,  char * buffer, ushort contentLen);
    bool getContentSlow(int *fd, ushort type,  char * buffer, ushort contentLen);
    bool getContentMedium(int *fd, ushort type,  char * buffer, ushort contentLen);
    void redirectToCallback(std::string eventName, Protocol::Sx_packet packet, Player * player);
    Player* retrieveClientUDP(int listener);
    Player* retrieveClientTCP(int listener);
    ushort checkFdType(int fd);
    bool addNewConnection(int fd);
    void closeAllConnections();
    ushort getPacketLength(int *fd,ushort type);
    Player * createClient(sockaddr_in socketParam);
 
public:
    const static short int TCP = 1;
    const static short int UDP = 2;

    void init();
    void serverStart();
    void stop();
    void addListener(Listener listener);
    void removeListener(int position);
    void addClientManager(PlayerManager * playerManager);
    Listener createListener(short int type, unsigned short int port, short timeout);   

    void setEventListener(std::string eventName, SxPacketCallback callback);

};


