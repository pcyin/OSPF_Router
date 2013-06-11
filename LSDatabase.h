#ifndef LSDATABASE_H
#define LSDATABASE_H
#include <vector>
#include <stdlib.h>
#include "Packets.h"
#include <pthread.h>

using namespace std;

class LSDatabase
{
    public:
        LSDatabase();
		static RouterLSA *getRouterLSAById(uint32_t linkId,uint32_t adr_id);
		static NetworkLSA *getNetworkLSAById(uint32_t linkId,uint32_t adr_id);
		static bool remove(uint32_t linkId, uint32_t adr_id, int type);
		static void insertRouterLSA(RouterLSA lsa);
		static void insertNetworkLSA(NetworkLSA lsa);
		static RouterLSA *getRouterLSAByAdrId(uint32_t adr_id);
		static NetworkLSA *getNetworkLSAByAdrId(uint32_t adr_id);
		static void removeByAdrId( uint32_t ad_router, int type );
		static RouterLSA *getRouterLSAByLinkStateId(uint32_t linkId);
		static NetworkLSA * getNetworkLSAByLinkStateId( uint32_t linkId );
		static pthread_mutex_t rlock;
		static pthread_mutex_t nlock;
		static pthread_mutex_t slock;
        static vector<RouterLSA> routerLSAList;
        static vector<NetworkLSA> networkLSAList;
    protected:
    private:
};

#endif // LSDATABASE_H
