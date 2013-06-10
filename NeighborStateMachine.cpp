#include "NeighborStateMachine.h"
#include <iostream>
using namespace std;

NeighborStateMachine::NeighborStateMachine()
{
    currentState = (NBR_ST_DOWN);
    actionPtrMap[NBR_EVT_HELLORECV] = (ActionPtr)(&NeighborStateMachine::onHelloRecved);
    actionPtrMap[NBR_EVT_2WAYRECV] = (ActionPtr)(&NeighborStateMachine::on2wayRecved);
}

void NeighborStateMachine::onHelloRecved(Sender& sender,EventArgs& args){
    cout<<"Hello Recved From Nbr "<<endl;
    if(currentState == NBR_ST_DOWN){
        currentState = NBR_ST_INIT;
    }
}

void NeighborStateMachine::on2wayRecved(Sender& sender,EventArgs& args){
    cout<<"2Way Recved From Nbr "<<endl;
}
