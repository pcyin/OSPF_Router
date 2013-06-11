#include "LSDatabase.h"

LSDatabase::LSDatabase()
{
    //ctor
}

RouterLSA * LSDatabase::getRouterLSAById( uint32_t linkId,uint32_t adr_id )
{
    for(vector<RouterLSA>::iterator it = routerLSAList.begin();it!=routerLSAList.end();it++){
        if(it->header.state_id == linkId && it->header.ad_router == adr_id)
            return &*it;
    }
    return NULL;
}

RouterLSA *LSDatabase::getRouterLSAByAdrId(uint32_t adr_id){
	for(vector<RouterLSA>::iterator it = routerLSAList.begin();it!=routerLSAList.end();it++){
		if(it->header.ad_router == adr_id)
			return &*it;
	}
	return NULL;

}

NetworkLSA *LSDatabase::getNetworkLSAByAdrId(uint32_t adr_id){
	for(vector<NetworkLSA>::iterator it = networkLSAList.begin();it!=networkLSAList.end();it++){
		if(it->header.state_id == adr_id)
			return &*it;
	}
	return NULL;
}

RouterLSA * LSDatabase::getRouterLSAByLinkStateId( uint32_t linkId )
{
	for(vector<RouterLSA>::iterator it = routerLSAList.begin();it!=routerLSAList.end();it++){
		if(it->header.state_id == linkId)
			return &*it;
	}
	return NULL;

}

NetworkLSA * LSDatabase::getNetworkLSAByLinkStateId( uint32_t linkId )
{
	for(vector<NetworkLSA>::iterator it = networkLSAList.begin();it!=networkLSAList.end();it++){
		if(it->header.state_id == linkId)
			return &*it;
	}
	return NULL;
}

NetworkLSA * LSDatabase::getNetworkLSAById( uint32_t linkId,uint32_t adr_id )
{
    for(vector<NetworkLSA>::iterator it = networkLSAList.begin();it!=networkLSAList.end();it++){
        if(it->header.state_id == linkId && it->header.ad_router == adr_id)
            return &*it;
    }
    return NULL;
}

bool LSDatabase::remove( uint32_t linkId, uint32_t adr_id, int type )
{
	bool flag = false;
	if(type==1){
		pthread_mutex_lock(&rlock);
		for(vector<RouterLSA>::iterator it = routerLSAList.begin();it!=routerLSAList.end();it++){
			if(it->header.state_id == linkId && it->header.ad_router){
				routerLSAList.erase(it);
				flag = true;
				break;
			}
		}
		pthread_mutex_unlock(&rlock);
	}else if(type == 2){
		pthread_mutex_lock(&rlock);
		for(vector<NetworkLSA>::iterator it = networkLSAList.begin();it!=networkLSAList.end();it++){
			if(it->header.state_id == linkId && it->header.ad_router){
				networkLSAList.erase(it);
				flag = true;
				break;
			}
		}
		pthread_mutex_unlock(&rlock);
	}
	return flag;
}

void LSDatabase::insertRouterLSA(RouterLSA lsa){
	pthread_mutex_lock(&rlock);
	routerLSAList.push_back(lsa);
	pthread_mutex_unlock(&rlock);
}

void LSDatabase::removeByAdrId( uint32_t ad_router, int type )
{
	//throw std::exception("The method or operation is not implemented.");
}

void LSDatabase::insertNetworkLSA( NetworkLSA lsa )
{
	pthread_mutex_lock(&rlock);
	networkLSAList.push_back(lsa);
	pthread_mutex_unlock(&rlock);
}


