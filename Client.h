#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
#include "Protocol.h"


typedef struct Datacache
{
   
    const static ushort STAGE_0 = 0;
    const static ushort STAGE_1 = 1;
    const static ushort STAGE_2 = 2;
    const static ushort STAGE_3 = 3;
    const static ushort COMPLETED = 4;

    Protocol::Sx_packet packet;
    ushort state = Datacache::STAGE_0;    
};

class Client 
{
    public:
        Client(uint32_t address, ushort port);
        std::string getIP();
        ushort getPort();
        void setPreviousData();
        Datacache * getCurrentDatacache();
        
    private:
        uint32_t address;
        ushort port;
        struct Datacache *previousData;
        struct Datacache *currentData;
    

};