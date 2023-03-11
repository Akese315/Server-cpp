#pragma once
#include <unordered_map>
#include "Protocol.h"
#include "string"
#include "Console.h"
#include "Player.h"

typedef void (*SxPacketCallback)(Protocol::Sx_packet packet, Player * player);


class EventManager 
{

    public:
    SxPacketCallback getCallbackByName(std::string name);
    void addCallback(std::string name, SxPacketCallback callback);


    std::unordered_map<std::string,SxPacketCallback> Events;
};