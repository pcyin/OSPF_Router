#include "Timers.h"
#include "Sender.h"
#include "Interface.h"
#include "EventArgs.h"
#include <unistd.h>
#include "InterfaceEventState.h"

void* Timers::waitTimer(void* inter){
    Interface* interface = (Interface*)inter;

    sleep(40);
    EventArgs args;
    args.eventId = IT_EVT_WAITTIMER;
    interface->invokeEvent(WAIT_TIMER,args);
}
