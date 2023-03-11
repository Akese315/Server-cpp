#include <stdio.h>
#include <string.h>
#include <iostream>


typedef struct
{
    ushort eventShort = 1;
    float x, y, z, rx, ry, rz;
} Position_t;

typedef struct
{
    ushort eventShort = 2;
    ushort state;
} State_t;

typedef struct
{
    ushort eventShort = 3;
    char *text;
} Message_t;

class Event
{

public:
    struct StateValue
    {
        const static short RunningState = 1;
        const static short CrounchingState = 2;
        const static short WalkingState = 3;
        const static short SwimingState = 4;
    };

    const static ushort EventLen = 2;
    const static ushort PosiLen = 24;
    const static ushort StateLen = 2;
    const static ushort MessageLen = 64;

    const static ushort positionEvent = 1;
    const static ushort stateEvent = 2;
    const static ushort messageEvent = 3;

    static char *getEventName(ushort myEvent);
    static ushort getEventByte(ushort myEvent);

    static void setPosition(Position_t *position, float *coordinates);
    static void setState(State_t *state, ushort stateNumber);
    static void setMessage(Message_t *message, const char *text);
};
