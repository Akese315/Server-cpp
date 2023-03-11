#pragma once
#include <unordered_map>
#include "Player.h"
#include <netinet/in.h>
#include <iostream>

class PlayerManager
{
    public:
        PlayerManager();
        void addPlayer(uint32_t address, Player * player);
        void addPlayer(int fd, Player * player);
        void removePlayer(uint32_t address);
        
        Player* getPlayer(uint32_t address);
        Player* getPlayer(int fd);
    private:
        std::unordered_map<uint32_t,Player*> addrPlayerMap;
        std::unordered_map<int,Player*>fdPlayerMap;

};