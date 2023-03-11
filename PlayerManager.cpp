#include "PlayerManager.h"

PlayerManager::PlayerManager()
{   
    addrPlayerMap = {};
    fdPlayerMap = {};
}

void PlayerManager::addPlayer(uint32_t address, Player * player)
{         
    this->addrPlayerMap.insert({address,player});
}

void PlayerManager::addPlayer(int fd, Player * player)
{       
    this->fdPlayerMap.insert({fd, player});
}

void PlayerManager::removePlayer(uint32_t address)
{
    this->addrPlayerMap.erase(address);
}

Player* PlayerManager::getPlayer(uint32_t address)
{
    std::unordered_map<uint32_t,Player*>::const_iterator got = this->addrPlayerMap.find(address);
    if(got==this->addrPlayerMap.end())
    {
        return nullptr;
    }
    else return got->second;
}

Player* PlayerManager::getPlayer(int fd)
{
    std::unordered_map<int,Player*>::const_iterator got = this->fdPlayerMap.find(fd);
    if(got==this->fdPlayerMap.end())
    {
        return nullptr;
    }
    else return got->second;
}