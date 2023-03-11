#pragma once
#include <queue>
#include <thread>
#include "Console.h"
#include <vector>
#include "Protocol.h"
#include "Player.h"

class ThreadPool
{
    public:

        void start(void(*callback)(std::string eventName, Protocol::Sx_packet packet, Player * player));
        

    private:    
        const int MaxThread = 20;
        std::thread * threads;
        
};