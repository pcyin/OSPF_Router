#ifndef NEIGHBORSTATEMACHINE_H
#define NEIGHBORSTATEMACHINE_H

#include <stdio.h>
#include "StateMachine.h"
#include "NeighborEventState.h"

class NeighborStateMachine : public StateMachine
{
    public:
        NeighborStateMachine();
    protected:
    private:
        void onHelloRecved(Sender&,EventArgs&);
        void on2wayRecved(Sender&,EventArgs&);
};

#endif // NEIGHBORSTATEMACHINE_H
