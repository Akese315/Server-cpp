#include "EventData.h"

char* Event::getEventName(ushort myevent)
{
    switch(myevent)
    {
        case Event::positionEvent:

            return (char*)"position";
        case Event::stateEvent:
            return (char*)"state";
        case Event::messageEvent:
            return (char*)"message";
        default:
            return (char*)"no event";
    }
}

ushort Event::getEventByte(ushort myevent)
{
    switch(myevent)
    {
        case Event::positionEvent:
            return Event::PosiLen;
        case Event::stateEvent:
            return Event::StateLen;
        case Event::messageEvent:
            return Event::MessageLen;
        default:
            return 0;
    }
}

void Event::setMessage(Message_t * message,const char * text)
{
    std::cout <<"is about to set message\n";
    memcpy(message+2,text,sizeof(text));
}

void Event::setPosition(Position_t * position, float * coordinates)
{
    memcpy(position+2,coordinates,sizeof(coordinates));
}

void Event::setState(State_t * state, ushort stateNumber)
{
    state->state = stateNumber;
}



