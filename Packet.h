typedef struct Packet
{
    unsigned short length;
    unsigned short event;
    void *content;
};