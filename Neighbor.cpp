#include "Neighbor.h"
#include "PacketManager.h"
#include "NeighborEventState.h"
#include "LSDatabase.h"
#include "LSAManager.h"
#include "Interface.h"

using namespace std;

Neighbor::Neighbor(in_addr_t ip):
	ip(ip),id(0),isMaster(false),priority(1),last_seq_num(0),lastDD_len(0)
{
    currentState = (NBR_ST_DOWN);
    actionPtrMap[NBR_EVT_HELLORECV] = (ActionPtr)(&Neighbor::onHelloRecved);
    actionPtrMap[NBR_EVT_2WAYRECV] = (ActionPtr)(&Neighbor::on2wayRecved);
    actionPtrMap[NBR_EVT_1WAY] = (ActionPtr)(&Neighbor::on1wayRecved);
    actionPtrMap[NBR_EVT_NEGOTIATIONDONE] = (ActionPtr)(&Neighbor::onNegotiationDone);
    actionPtrMap[NBR_EVT_EXCHANGEDONE] = (ActionPtr)(&Neighbor::onExchageDone);

}

void Neighbor::onHelloRecved(Sender& sender,EventArgs& args){
    cout<<"Hello Recved From Nbr "<<endl;
    if(currentState == NBR_ST_DOWN){
        currentState = NBR_ST_INIT;
        cout<<"NBR_ST_DOWN -> NBR_ST_INIT"<<endl;
    }
}

void Neighbor::on2wayRecved(Sender& sender,EventArgs& args){
    cout<<"2Way Recved From Nbr "<<endl;
    if(currentState == NBR_ST_INIT){
        currentState = NBR_ST_EXSTART;
        ddNum = 0;
        isMaster = true;
        pthread_create(&ddSendThread,NULL,PacketManager::sendEmptyDDPacets,(void *)this);
        //PacketManager::sendDDPackets(this);
        cout<<"NBR_ST_INIT -> NBR_ST_EXSTART"<<endl;
    }
}

void Neighbor::initSummaryList(){
	pthread_mutex_lock(&LSDatabase::rlock);

	for(vector<RouterLSA>::iterator it = LSDatabase::routerLSAList.begin() ; it!=LSDatabase::routerLSAList.end() ; it++ ){
		summaryList.push_back((*it).header);
	}
	for(vector<NetworkLSA>::iterator it = LSDatabase::networkLSAList.begin() ; it!=LSDatabase::networkLSAList.end() ; it++ ){
		summaryList.push_back((*it).header);
	}

	pthread_mutex_unlock(&LSDatabase::rlock);
}

void Neighbor::on1wayRecved(Sender& sender,EventArgs& args){
    cout<<"1Way Recved From Nbr "<<endl;
    if(currentState>=NBR_ST_2WAY){
        currentState = NBR_ST_INIT;
        cout<<"NBR_ST_2WAY -> NBR_ST_INIT"<<endl;
    }
}

void Neighbor::onNegotiationDone(Sender& sender,EventArgs& args){
    cout<<"onNegotiationDone Recved From Nbr "<<endl;
    if(currentState>=NBR_ST_EXSTART){
        //pthread_cancel(ddSendThread);
        currentState = NBR_ST_EXCHANGE;
        cout<<"NBR_ST_EXSTART -> NBR_ST_EXCHANGE"<<endl;
		initSummaryList();
    }
}

void Neighbor::onExchageDone(Sender& sender,EventArgs& args){
    cout<<"onExchageDone Recved From Nbr "<<endl;
    if(currentState>=NBR_ST_EXCHANGE){
        if(this->reqList.size() == 0){
            currentState = NBR_ST_FULL;
			if(this->interface->dr == this->interface->ip)
				LSAManager::generateNetworkLSA(this->interface);

            cout<<"NBR_ST_EXCHANGE -> NBR_ST_FULL"<<endl;
        }else{
            currentState = NBR_ST_LOADING;
            cout<<"NBR_ST_EXCHANGE -> NBR_ST_LOADING"<<endl;
            pthread_t pid;
            pthread_create(&pid,NULL,PacketManager::sendLSReq,this);
        }
    }
}

