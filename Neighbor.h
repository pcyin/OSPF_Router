#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <pthread.h>
#include "StateMachine.h"
#include <list>
#include "Packets.h"

class PacketManager;
class Interface;
class Neighbor : public StateMachine
{
    public:
        Neighbor(in_addr_t);
        bool isMaster;
        int ddNum;
        uint32_t id;
        uint32_t ip;
        uint32_t ndr;
        uint32_t nbdr;
        int priority;
        Interface *interface;
        pthread_t ddSendThread;
        list<ospf_lsa_header> reqList;
		list<ospf_lsa_header> summaryList;
		int last_seq_num;
		int lastDD_len;
		char lastDD[MAX_PACKET_LEN];
		void initSummaryList();
    protected:
    private:
        void onHelloRecved(Sender&,EventArgs&);
        void on2wayRecved(Sender&,EventArgs&);
        void on1wayRecved(Sender&,EventArgs&);
        void onNegotiationDone(Sender&,EventArgs&);
        void onExchageDone(Sender& sender,EventArgs& args);
};

#endif // NEIGHBOR_H
