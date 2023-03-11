#include "EventManager.h"

SxPacketCallback EventManager::getCallbackByName(std::string name)
{
    if(name.empty())
    {
        Console::printError("Name is NULL when searching for an Event");
    }
    std::unordered_map<std::string, SxPacketCallback>::iterator iterator = Events.find(name);
    if(iterator == Events.end())
    {
        return nullptr;
    }

   return iterator->second;
}

void EventManager::addCallback(std::string name, SxPacketCallback callback)
{
    if(name.empty()||callback == nullptr)
    {
        Console::printError("Name or Callback is NULL when adding an Event");
    }
    std::pair<std::string, SxPacketCallback> pair(name,callback);
    Events.insert(pair);
}