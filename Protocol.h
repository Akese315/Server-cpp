#pragma once
#include "stdio.h"
#include <string.h>

class Protocol
{
    public:
    
    typedef struct packet
    {
        unsigned short length;
        char * content;
    };

    typedef struct Sx_packet
    {
        packet event;
        packet body;
    };

    

    static char* toBuffer(Sx_packet packet);
    static Sx_packet toPacket(char * buffer);    
    

};