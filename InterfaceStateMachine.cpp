#include "InterfaceStateMachine.h"

InterfaceStateMachine::InterfaceStateMachine(Interface* inter){
    this->interface = inter;
	actionPtrMap[IT_EVT_INTERFACEUP] = (void (StateMachine::*)(Sender&,EventArgs&))(&InterfaceStateMachine::onInterfaceUp);
	actionPtrMap[IT_EVT_WAITTIMER] = (void (StateMachine::*)(Sender&,EventArgs&))(&InterfaceStateMachine::onWaitTimer);

}

void InterfaceStateMachine::onInterfaceUp(Sender& sender,EventArgs& args){
	if(currentState==IT_ST_DOWN){
		currentState = IT_ST_WAITING;
	}
}

void InterfaceStateMachine::onWaitTimer(Sender& sender,EventArgs& args){
	if(currentState==IT_ST_WAITING){
        interface->calcDR();
	}
}

