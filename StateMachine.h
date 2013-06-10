#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <stdio.h>
#include "Sender.h"
#include "EventArgs.h"



// base class for state machines
class StateMachine
{
public:
	StateMachine():
		currentState(0){};
	void invokeEvent(Sender&,EventArgs&);
	int currentState;
	void (StateMachine::*actionPtrMap[10])(Sender&,EventArgs&);
private:

};


typedef void (StateMachine::*ActionPtr)(Sender&,EventArgs&);
#endif //STATE_MACHINE_H
