#ifndef LSAMANAGER_H
#define LSAMANAGER_H
#include <vector>
#include <list>
#include "Packets.h"

using namespace std;
class Interface;
struct RouterLSA;
class LSAManager
{
    public:
        LSAManager();
        static RouterLSA generateRouterLSA(vector<Interface*> inters);
        static void *lsaGenerate(void *null);
		static NetworkLSA generateNetworkLSA(Interface* interface);
        static void onRouterLSAGenerate();
        static void onNetworkLSAGenerate(Interface* interface);
        static pthread_mutex_t rlock;
        static pthread_mutex_t nlock;
        static list<RouterLSA> rlsaList;
        static list<NetworkLSA> nlsaList;
    protected:
    private:
};

#endif // LSAMANAGER_H
