#include "Protocol.h"

char * Protocol::toBuffer(Sx_packet packet)
{
    unsigned short total_length = 0;
    total_length += 2 ;// ushort
    total_length += packet.event.length;
    total_length += 2; // ushort
    total_length += packet.body.length;

    char buffer[total_length];

    memcpy(buffer,&packet,(size_t)total_length);

    return buffer;
}


Protocol::Sx_packet Protocol::toPacket(char * buffer)
{
   unsigned short total_length =  sizeof(buffer);

   struct Sx_packet packet;

   memcpy(&packet, buffer, sizeof(short));
   memcpy(&packet, buffer + 2, packet.event.length);
   memcpy(&packet, buffer + packet.event.length + 2, 2);
   memcpy(&packet, buffer + 2 + packet.body.length + 2, packet.body.length);

   return packet;
}