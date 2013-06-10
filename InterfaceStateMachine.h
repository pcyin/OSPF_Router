#ifndef IT_STATE_MACHINE_H
#define IT_STATE_MACHINE_H
#include <stdio.h>
#include "StateMachine.h"
#include "InterfaceEventState.h"

class Interface;

class InterfaceStateMachine:public StateMachine
{
public:
	InterfaceStateMachine(Interface*);
private:
	void onInterfaceUp(Sender&,EventArgs&);
	void onWaitTimer(Sender&,EventArgs&);
	void onBackUpSeen(Sender&,EventArgs&);
	Interface* interface;
};

#endif //IT_STATE_MACHINE_H
