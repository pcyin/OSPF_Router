#include "LSAManager.h"
#include "Packets.h"
#include "Interface.h"
#include "InterfaceEventState.h"
#include "Neighbor.h"
#include "NeighborEventState.h"
#include <unistd.h>
#include "LSDatabase.h"
#include "Config.h"

LSAManager::LSAManager()
{
    //ctor
}

void *LSAManager::lsaGenerate(void *null){
    while(true){
        pthread_mutex_lock(&rlock);
        if(rlsaList.size() > 0){
			RouterLSA lsa = *rlsaList.begin();
			LSDatabase::remove(lsa.header.state_id,lsa.header.ad_router,1);
			rlsaList.pop_front();
			LSDatabase::insertRouterLSA(lsa);
        }
        pthread_mutex_unlock(&rlock);
        sleep(5);
    }
}

void LSAManager::onRouterLSAGenerate(){
    RouterLSA lsa = generateRouterLSA(Config::inters);
    RouterLSA *ptr;
    if( (ptr = LSDatabase::getRouterLSAById(Config::routerId,Config::routerId)) == NULL ){
        pthread_mutex_lock(&rlock);
        rlsaList.push_back(lsa);
        pthread_mutex_unlock(&rlock);
    }else{
        if(*ptr != lsa){
            pthread_mutex_lock(&rlock);
            rlsaList.push_back(lsa);
            pthread_mutex_unlock(&rlock);
        }
    }
}

void LSAManager::onNetworkLSAGenerate(Interface* interface){
	NetworkLSA lsa = generateNetworkLSA(interface);
    NetworkLSA *ptr;
    if( (ptr = LSDatabase::getNetworkLSAById(Config::routerId,Config::routerId)) == NULL ){
        pthread_mutex_lock(&nlock);
        nlsaList.push_back(lsa);
        pthread_mutex_unlock(&nlock);
    }else{
        if(*ptr != lsa){
            pthread_mutex_lock(&nlock);
            nlsaList.push_back(lsa);
            pthread_mutex_unlock(&nlock);
        }
    }
}

RouterLSA LSAManager::generateRouterLSA(vector<Interface*> inters)
{
    RouterLSA lsa;
    for(vector<Interface*>::iterator it = inters.begin() ; it!=inters.end() ; it++ )
    {
        Interface *inter = *it;
        if(inter->currentState != IT_ST_DOWN)
        {
            RouterLSALink link;
            if(inter->currentState == IT_ST_WAITING)
            {
                link.type = 3;
                link.linkId = inter->ip & inter->mask;
                link.linkData = inter->mask;
                link.metric = inter->cost;
            }
            else
            {
                //have dr
                if(inter->dr == inter->ip || inter->getNbrByIp(inter->dr)->currentState == NBR_ST_FULL)
                {
                    link.type = 2;
                    link.linkId = inter->dr;
                    link.linkData = inter->ip;
                    link.metric = inter->cost;
                }
                else
                {
                    link.type = 3;
                    link.linkId = inter->ip & inter->mask;
                    link.linkData = inter->mask;
                    link.metric = inter->cost;
                }
            }
            lsa.links.push_back(link);
        }
    }
    lsa.header.ad_router = Config::routerId;
    lsa.header.type = 1;
    lsa.header.state_id = Config::routerId;
    return lsa;
}

NetworkLSA LSAManager::generateNetworkLSA( Interface* interface )
{
	NetworkLSA lsa;
	lsa.header.state_id = interface->ip;
	lsa.header.ad_router = Config::routerId;
	for(list<Neighbor*>::iterator it = interface->nbrList.begin();it!=interface->nbrList.end();it++){
		if((*it)->currentState==NBR_ST_FULL)
			lsa.routers.push_back((*it)->id);
	}
	return lsa;
}
