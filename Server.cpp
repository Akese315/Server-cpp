#include "Server.h" 

void Server::init()
{
    //threadPool.start(Server::redirectToCallback);

    if(ListenerArray.empty())
    {
        Console::printError("No listeners set");
        canStart = false;
        return;
    }

    memset(&fds,0, sizeof(&fds));

    int on =  1;

    //Set in File descriptor all listeners
    for(int i = 0; i< ListenerArray.size();i++)
    {
        setsockopt(ListenerArray[i].socket, SOL_SOCKET,  SO_REUSEADDR,(char *)&on,(socklen_t)sizeof(on)); 
        fcntl(ListenerArray[i].socket, F_SETFL, O_NONBLOCK);
        fds[i].events = POLLIN;
        fds[i].fd = ListenerArray[i].socket;
        activeFds++;
    }

    //Bind all listeners
    for(int i = 0; i < ListenerArray.size();i++)
    {
        sockaddr_in bindParams;
        bindParams.sin_family =AF_INET;
        bindParams.sin_port = htons(ListenerArray[i].port);
        inet_aton("127.0.0.1",&bindParams.sin_addr);
        int error = bind(ListenerArray[i].socket,(struct sockaddr *)&bindParams,(socklen_t)sizeof(bindParams));
        if(error != 0)
        {
            getError("bind");
            canStart = false;
            return;
        }

        std::string info = "server bind on port : "+std::to_string(ListenerArray[i].port);
        Console::printInfo(info);
    }
    
}

Listener Server::createListener(short int type, unsigned short int port, short timeout)
{
    
    Listener listener = {};
    listener.port = port;
    listener.type = type;
    listener.timeout = timeout;
    
    if(type == Server::TCP)
    {
        listener.socket = socket(AF_INET,SOCK_STREAM,0);
    }else if(type == Server::UDP)
    {
        listener.socket = socket(AF_INET,SOCK_DGRAM,0);
    }
    if(listener.socket == -1 || listener.socket == 0)
    {
        getError("create listener");
    }
    Console::printInfo("socket created : "+ std::to_string (listener.socket));
    return listener;
}

void Server::serverStart()
{
    Console::printSuccess("Server started successfully.");

    if(&playerManager == nullptr)
    {
        Console::printWarning("can\'t start because the server has no playerManager");
        return;
    }
    if(!canStart)
    {
        Console::printWarning("can\'t start");
        return;
    }
    for(int i = 0; i< ListenerArray.size();i++)
    {
        listen(ListenerArray[i].socket, 5);
    }

    Console::printInfo("Listening..."); 
    do{
        int fdr = poll(fds,activeFds,-1);
        if (fdr < 0)
        {
            getError("poll");
            break;
        }
        if(fdr == 0)
        {
            Console::printWarning("Time out reached");
            break;
        }
        if(fdr > 0)
        {
            checkFDStatus(fdr);
        }
    }while(true);
}

void Server::checkFDStatus(int fdr)
{
    for(int i = (activeFds -1); i >= 0; i--)
    {
        if(fds[i].revents == 0)
        {
            continue;
        }

        if(fds[i].revents != POLLIN)
        {
            std::cout <<" Revents : "<<fds[i].revents<< ", fd : "<<fds[i].fd<<std::endl;            
            getError("revents");
            continue;
        }
        bool isListener = false;

        //We have to check if it's a TCP listener

        ushort type = checkFdType(fds[i].fd);
        if(type == Server::TCP_LISTENER)
        {
            addNewConnection(fds[i].fd);
            continue;
        }   

        Player * player;
        if(type == Server::UDP_SOCKET)
        {
            player = retrieveClientUDP(fds[i].fd);
        }
        else if (type == Server::TCP_SOCKET)
        {
            player = retrieveClientTCP(fds[i].fd);
        }

        Receive(player,&fds[i].fd, type);       
    }
}

ushort Server::checkFdType(int fd)
{
    //if fd is a TCP listener → return 1
    //if fd is a UDP socket → return 2
    //if fd is a TCP socket → return 3

    for(int SocketIndex = 0; SocketIndex< ListenerArray.size();SocketIndex++)
    {
        if(fd == ListenerArray[SocketIndex].socket)    
        {
            if(ListenerArray[SocketIndex].type == Server::TCP)
            {
                return 1;
            } 
            if(ListenerArray[SocketIndex].type == Server::UDP)
            {
                return 2;
            }                       
        }
        else
        {
            return 3;
        } 
    }    
}

Player* Server::retrieveClientUDP(int listener)
{
    char * buffer;
    sockaddr_in addr = {};
    recvfrom(listener,buffer,1,MSG_PEEK,(sockaddr*)&addr,(socklen_t *)sizeof(addr));//read header
    Player* player = playerManager->getPlayer(addr.sin_addr.s_addr);
    return player;
}

Player* Server::retrieveClientTCP(int listener)
{
   Player* player = playerManager->getPlayer(listener);
   return player;
}

void Server::Receive(Player* player,int *fd, ushort type)
{
    //data incoming
    Datacache * cache = player->getCurrentDatacache();

    if(cache->state == Datacache::STAGE_0)
    {
        ushort EventLength = getPacketLength(fd,type);
        if(EventLength == 0)return;       
        cache->state = Datacache::STAGE_1;
        cache->packet.event.length = EventLength;      
    }
    if(cache->state == Datacache::STAGE_1)
    {
        Console::printInfo("Stage 1"); 
        
        bool response = getContentMedium(fd, type, cache->packet.event.content,cache->packet.event.length);
        if(!response)
        {
            return;
        }
        cache->state = Datacache::STAGE_2;     
    }
    if(cache->state == Datacache::STAGE_2)
    {
        Console::printInfo("Stage 2");   
        ushort BodyLength = getPacketLength(fd,type);
        if(BodyLength == 0)return;       
        cache->state = Datacache::STAGE_3;
        cache->packet.body.length = BodyLength;            
    }
    if(cache->state == Datacache::STAGE_3)
    {
        Console::printInfo("Stage 3"); 
        bool response = getContentMedium(fd, type, cache->packet.body.content,cache->packet.body.length);
        if(!response)
        {
            return;
        }
        cache->state = Datacache::COMPLETED;       
    }
    if(cache->state == Datacache::COMPLETED)
    {
        Console::printInfo("COMPLETED"); 

        std::string eventName = (std::string) cache->packet.event.content;

        
        
    }
}

void Server::Send(Player * player, int fd, ushort type, const char * buffer)
{
    if(type == Server::TCP)
    {
        char * newbuffer = (char*) buffer;      
        Console::printWarning("fd : ");
        int byte_sent = send(fd,newbuffer,(size_t) sizeof(newbuffer),MSG_NOSIGNAL);
        if(byte_sent == -1)
        {
            if(errno == EPIPE)
            {
                Console::printError("EPIPE");
            }
        }else if(byte_sent < sizeof(newbuffer)){
            Console::printError("Partial send");
        }else
        {
            Console::printInfo("sent");    
        }            
    }else
    if(type == Server::UDP)
    {
        sockaddr_in addr = {};
        addr.sin_port = player->getPort();
        addr.sin_family = AF_INET;
        inet_aton(player->getIP().c_str(),&addr.sin_addr);
        sendto(fd,buffer,sizeof(buffer),0,(sockaddr*)&addr,(socklen_t)sizeof(addr));
    }
}

ushort Server::getPacketLength(int *fd,ushort type)
{
    char length[Event::EventLen];
    if(type == Server::TCP_SOCKET)
    { 
        int bytes = recv(*fd,length,(size_t) Event::EventLen, NULL);
        if(bytes == 0)
        {
            Disconnected(fd);
            return 0;
        }
    }else
    {
        int bytes = recvfrom(*fd,length,(size_t) Event::EventLen,0,NULL, NULL);
    }

    std::bitset<16> third((
    std::bitset<8>(length[1])).to_string()+
    std::bitset<8U>(length[0]).to_string());
    
    return (ushort)third.to_ulong();
}



bool Server::getContentFast(int * fd, ushort type, char * buffer, ushort contentLen)
{
    int bytes;
    char tempBuffer[contentLen];
    if(type == Server::TCP_SOCKET)
    {
        bytes = recv(*fd,tempBuffer,contentLen, MSG_PEEK); //MSG_WAITALL peut être
        if(bytes == contentLen)
        {
            recv(*fd,buffer,contentLen,0);
            return true;
        }
        else
        {
            return false;
        }
        if(bytes == 0)
        {
            Disconnected(fd);
            return false;
        }        
    }
    else
    {
       bytes = recvfrom(*fd,buffer,contentLen,0,NULL, NULL);
       return true;
    }    
}

bool Server::getContentMedium(int *fd, ushort type,  char * buffer, ushort contentLen)
{
    int bytes;
    size_t length = 0;
    if(buffer != nullptr)
    {
        length = strlen(buffer);
    }        
    size_t lenRemaining = (size_t) contentLen - length;
    char tempBuffer[lenRemaining];

    if(type == Server::TCP_SOCKET)
    {        
        bytes = recv(*fd,tempBuffer,lenRemaining, MSG_DONTWAIT); 

        if(bytes == 0)
        {
            Disconnected(fd);
            return false;
        }     

    }else
    {
        bytes = recvfrom(*fd,tempBuffer,lenRemaining,0,NULL, NULL);
        
    }

    char newbuffer[contentLen];
    memcpy(newbuffer,buffer,length);
    memcpy(newbuffer+length, tempBuffer, bytes);
    buffer = newbuffer;
    if(bytes == lenRemaining)
    {
        return true;
    }
    else
    {
        return false;
    }    
}

bool Server::getContentSlow(int *fd, ushort type,  char * buffer, ushort contentLen)
{
    int bytes;
    char tempBuffer[contentLen];
    if(type == Server::TCP_SOCKET)
    {
        bytes = recv(*fd,tempBuffer,contentLen, MSG_WAITALL); //MSG_WAITALL peut être
        if(bytes == 0)
        {
            Disconnected(fd);
            return false;
        }        
        else
        {
            return true;
        }
    }
    else
    {
       bytes = recvfrom(*fd,buffer,contentLen,0,NULL, NULL);
       return true;
    }
}

void Server::redirectToCallback(std::string eventName, Protocol::Sx_packet packet, Player * player)
{
    SxPacketCallback callback = eventManager.getCallbackByName(eventName);
    callback(packet, player);

    //ThreadPool HERE -->

}

void Server::addClientManager(PlayerManager * playerManager)
{
    this->playerManager = playerManager;
}

bool Server::addNewConnection(int fd)
{    
    Console::printSuccess("New connection !");
    struct sockaddr_in socketParam{};
    socklen_t socketaddrLen = sizeof(socketParam);
    int newFD = accept(fd,(struct sockaddr *)&socketParam,(socklen_t*)&socketaddrLen);
    if(newFD < 0)
    {
        Console::printError("error on accept socket");
        getError("accept");
        return false;
    }

    addFD(newFD);
    Player * player = createClient(socketParam);
    playerManager->addPlayer(socketParam.sin_addr.s_addr,player);
    playerManager->addPlayer(newFD,player);
    Message_t message;
    message.text = (char*) Server::WELCOME_MESSAGE;    
    Send(player,fd,Server::TCP,(const char*)&message); 
}

Player* Server::createClient(sockaddr_in socketParam)
{   
    Player * player = new Player(socketParam.sin_addr.s_addr,socketParam.sin_port);
    return player;
}

void Server::addFD(int fd)
{
    if(activeFds == MAX_FDS_SIZE)
    {
        return;
    }
    fds[activeFds].fd = fd;
    fds[activeFds].events = POLLIN;
    activeFds++;
}



void Server::addListener(Listener listener)
{
    ListenerArray.push_back(listener);

}

void Server::removeListener(int position)
{
    ListenerArray.erase(ListenerArray.begin()+position);
}

void Server::setEventListener(std::string eventName, SxPacketCallback callback)
{
    eventManager.addCallback(eventName,callback);
}

void Server::getError(const char* where)
{
    std::string error = strerror(errno)+(std::string)" in "+where;
    Console::printError(error);    
    exit(0);
} 

void Server::purgeFDs()
{
    pollfd newfds[sizeof(unsigned short)];
    for(unsigned short i = 1; i<activeFds; i++)
    {
        if(fcntl(fds[i].fd, F_GETFD) == FD_CLOEXEC)
        {
            continue;
        }
        newfds[i].fd = fds[i].fd;
        newfds[i].events = fds[i].events;
        newfds[i].revents = fds[i].revents;
    }    
    //fds = newfds;
}

void Server::closeAllConnections()
{
    for(unsigned short i = 1; i< activeFds; i++)
    {
        close(fds[i].fd);
    }
}

void Server::Disconnected(int *fd)
{
    Console::printError("Client disconnected");
    *fd = -1;
    close(*fd);    
}

void Server::stop()
{
    closeAllConnections();
    Console::printSuccess("Server stopped successfully");
    Log::closeFile();
}