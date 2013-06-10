#include <assert.h>
#include <iostream>
#include "StateMachine.h"
void StateMachine::invokeEvent(Sender& sender,EventArgs& args){
	(this->*actionPtrMap[args.eventId])(sender,args);
}
