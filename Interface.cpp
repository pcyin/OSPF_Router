#include "Interface.h"
#include "Config.h"
#include "NeighborEventState.h"
#include "Neighbor.h"
#include "InterfaceEventState.h"
#include "LSAManager.h"
#include "Timers.h"
#include "PacketRetransmitter.h"

Interface::Interface():
    helloInterval(10),routerDeadInterval(40),dr(0),bdr(0),mask(0xffffff00),areaId(0),routerPriority(1),mtu(1500),cost(0)
{
    actionPtrMap[IT_EVT_INTERFACEUP] = (ActionPtr)(&Interface::onInterfaceUp);
    actionPtrMap[IT_EVT_WAITTIMER] = (ActionPtr)(&Interface::onWaitTimer);
    actionPtrMap[IT_EVT_BACKUPSEEN] = (ActionPtr)(&Interface::onBackUpSeen);
    actionPtrMap[IT_EVT_NEIGHBORCHANGE] = (ActionPtr)(&Interface::onNbrChange);

	retransmitter = new PacketRetransmitter(this);
}

Neighbor* Interface::getNbrByIp(in_addr_t ip)
{
    for(list<Neighbor*>::iterator it = nbrList.begin(); it!=nbrList.end() ; it++)
    {
        if((*it)->ip == ip)
            return *it;
    }
    return NULL;
}

Neighbor* Interface::createNbr(in_addr_t ip)
{
    Neighbor *nbr = new Neighbor(ip);
    nbr->interface = this;
    nbrList.push_back (nbr);
    return nbr;
}

void Interface::calcDR()
{
    list<Neighbor*> rlist;
    for(list<Neighbor*>::iterator it = nbrList.begin(); it!=nbrList.end(); it++)
    {
        if((*it)->currentState>=NBR_ST_2WAY)
        {
            rlist.push_back(*it);
        }
    }
    Neighbor *self = new Neighbor(this->ip);
    self->nbdr = this->bdr;
    self->ndr = this->dr;
    self->ip = this->ip;
    self->id = Config::routerId;
    self->priority = this->routerPriority;

    rlist.push_back(self);

    Neighbor *bdr=NULL,*dr=NULL;

    for(int i=0; i<1; i++)
    {
        bdr = dr = NULL;
        //calc the bdr
        list<Neighbor*> bdreList;
        for(list<Neighbor*>::iterator it = rlist.begin(); it!=rlist.end(); it++)
        {
            if((*it)->ndr != (*it)->ip)
            {
                bdreList.push_back(*it);
            }
        }

        int bdrDecNum=0;
        Neighbor* bdrDecList[10];

        for(list<Neighbor*>::iterator it = bdreList.begin(); it!=bdreList.end(); it++)
        {
            if((*it)->nbdr == (*it)->ip)
            {
                bdrDecList[bdrDecNum++] = *it;
            }
        }

        Neighbor* temp;
        if(bdrDecNum == 0)
        {
            for(list<Neighbor*>::iterator it = bdreList.begin(); it!=bdreList.end(); it++)
            {
                bdrDecList[bdrDecNum++] = *it;
            }
        }

        for(int j=0; j<bdrDecNum; j++)
        {
            for(int k=j+1; k<bdrDecNum; k++)
            {
                if(bdrDecList[k]->priority > bdrDecList[j]->priority)
                {
                    temp = bdrDecList[j];
                    bdrDecList[j] = bdrDecList[k];
                    bdrDecList[k] = temp;
                }
            }
        }

        int ptr = 0;
        for(; ptr < bdrDecNum && bdrDecList[ptr]->priority == bdrDecList[0]->priority ; ptr++)
            ;
        ptr--;
        if(ptr>0)
        {
            uint32_t id = bdrDecList[0]->id;
            int num = ptr+1;
            for(int j=1; j<num; j++)
            {
                if(id < bdrDecList[j]->id)
                {
                    ptr = j;
                    id = bdrDecList[j]->id;
                }
            }
        }
        bdr = bdrDecList[ptr];

        //calc the dr

        int drDecNum=0;
        Neighbor* drDecList[10];

        for(list<Neighbor*>::iterator it = rlist.begin(); it!=rlist.end(); it++)
        {
            if((*it)->ndr == (*it)->ip)
            {
                drDecList[drDecNum++] = *it;
            }
        }

        if(drDecNum>0)
        {
            for(int j=0; j<drDecNum; j++)
            {
                for(int k=j+1; k<drDecNum; k++)
                {
                    if(drDecList[k]->priority > drDecList[j]->priority)
                    {
                        temp = drDecList[j];
                        drDecList[j] = drDecList[k];
                        drDecList[k] = temp;
                    }
                }
            }

            int ptr = 0;
            for(; ptr < drDecNum && drDecList[ptr]->priority == drDecList[0]->priority ; ptr++)
                ;
            ptr--;
            if(ptr>0)
            {
                uint32_t id = drDecList[0]->id;
                int num = ptr+1;
                for(int j=1; j<num; j++)
                {
                    if(id < drDecList[j]->id)
                    {
                        ptr = j;
                        id = drDecList[j]->id;
                    }
                }
            }
            dr = drDecList[ptr];
        }else{
            dr = bdr;
        }

        if(!(
            Config::routerId == dr->id ||
            Config::routerId == bdr->id ||
            (this->dr == this->ip) && (dr->ip != this->dr) ||
            (this->bdr == this->ip) && (bdr->ip != this->bdr)
        ))
            break;
    }
	/* change of DR should invoke generation of Network LSA  */
	if(this->dr != dr->ip){
		if(this->dr == this->ip){
			LSAManager::onNetworkLSAGenerate(this);
		}
	}
    this->dr = dr->ip;
    this->bdr = bdr->ip;
	
    cout<<"new dr"<<this->dr<<endl;
    cout<<"new bdr"<<this->bdr<<endl;
}

void Interface::onInterfaceUp(Sender& sender,EventArgs& args)
{
    if(currentState==IT_ST_DOWN)
    {
		pthread_t transid;
		pthread_create(&transid,NULL,PacketRetransmitter::run,this->retransmitter);

        cout << "IT_ST_DOWN -> IT_ST_WAITING" << endl;
        currentState = IT_ST_WAITING;
		pthread_t tid;
		pthread_create(&tid,NULL,Timers::waitTimer,this);
		LSAManager::onRouterLSAGenerate();
    }
}

void Interface::onWaitTimer(Sender& sender,EventArgs& args)
{
    if(currentState==IT_ST_WAITING)
    {
        this->calcDR();
        LSAManager::onRouterLSAGenerate();
    }
}

void Interface::onBackUpSeen(Sender& sender,EventArgs& args)
{
    if(currentState==IT_ST_WAITING)
    {
        this->calcDR();
		if(this->ip == this->dr){
			cout << "IT_ST_WAITING -> IT_ST_DR" << endl;
			currentState = IT_ST_DR;
		}else if(this->ip == this->bdr){
			cout << "IT_ST_WAITING -> IT_ST_BACKUP" << endl;
			currentState = IT_ST_BACKUP;
		}else{
			cout << "IT_ST_WAITING -> IT_ST_DROTHER" << endl;
			currentState = IT_ST_DROTHER;
		}

        LSAManager::onRouterLSAGenerate();
    }
}

void Interface::onNbrChange(Sender& sender,EventArgs& args)
{
    cout<<"nbr change"<<endl;
    this->calcDR();
    LSAManager::onRouterLSAGenerate();
}
