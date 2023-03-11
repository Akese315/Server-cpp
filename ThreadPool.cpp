#include "ThreadPool.h"


void ThreadPool::start(void(*callback)(std::string eventName, Protocol::Sx_packet packet, Player * player))
{

    threads = new std::thread[MaxThread];


    const uint32_t num_threads = std::thread::hardware_concurrency();
    std::string info = std::to_string(MaxThread) + "/" + std::to_string(num_threads) + " threads used";
   
    for(int i = 0; i< MaxThread; i++)
    {        
        threads[i] = std::thread(callback);
        threads[i].join();        
    }
}