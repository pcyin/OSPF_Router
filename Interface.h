#ifndef INTERFACE_H
#define INTERFACE_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdint.h>
#include <list>
#include <arpa/inet.h>
#include "StateMachine.h"

using namespace std;
class Neighbor;
class PacketRetransmitter;
class Interface: public StateMachine
{
    public:
        Interface();
        in_addr_t ip;
        uint32_t mask;
        uint32_t areaId;
        int helloInterval;
        int routerDeadInterval;
        int routerPriority;
        uint32_t dr;
        uint32_t bdr;
        list<Neighbor*> nbrList;
        Neighbor* getNbrByIp(in_addr_t);
        Neighbor* createNbr(in_addr_t);
		PacketRetransmitter* retransmitter;
        void calcDR();
        int mtu;
        int cost;
    protected:
    private:
    	void onInterfaceUp(Sender&,EventArgs&);
        void onWaitTimer(Sender&,EventArgs&);
        void onBackUpSeen(Sender&,EventArgs&);
        void onNbrChange(Sender&,EventArgs&);
};

#endif // INTERFACE_H
