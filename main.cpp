#include <iostream>
#include <pthread.h>
#include "Timer.h"
#include <unistd.h>
#include "Config.h"
#include "PacketManager.h"
#include "Interface.h"
#include "InterfaceEventState.h"
#include "LSDatabase.h"
#include "LSAManager.h"
#include "Common.h"

using namespace std;

/* beign init of static memebers */
uint32_t Config::routerId = ntohl(inet_addr("192.168.2.101"));
vector<RouterLSA> LSDatabase::routerLSAList;
vector<NetworkLSA> LSDatabase::networkLSAList;
pthread_mutex_t LSAManager::rlock;
pthread_mutex_t LSAManager::nlock;
pthread_mutex_t LSDatabase::rlock;

list<RouterLSA> LSAManager::rlsaList;
list<NetworkLSA> LSAManager::nlsaList;

vector<Interface*> Config::inters;
/* end init of static memebers */

int main()
{
    pthread_mutex_init(&LSAManager::rlock,NULL);
    pthread_mutex_init(&LSAManager::nlock,NULL);
	pthread_mutex_init(&LSDatabase::rlock,NULL);

    pthread_t recvThread;
    pthread_t helloSendThread;
    pthread_t helloRecvThread;
    pthread_t lsathread;
    Interface inter_eth1;
    inter_eth1.ip = ntohl(inet_addr("192.168.2.101"));
    Config::inters.push_back(&inter_eth1);
    //inter_eth1.dr=ntohl(inet_addr("192.168.2.1"));
   // inter_eth1.mask =
    EventArgs arg;
    arg.eventId = IT_EVT_INTERFACEUP;
    inter_eth1.invokeEvent(PACKET_RECV,arg);
    pthread_create(&helloRecvThread,NULL,PacketManager::recvHelloPacket,&inter_eth1);
    pthread_create(&helloSendThread,NULL,PacketManager::sendHello,&inter_eth1);
    pthread_create(&lsathread,NULL,LSAManager::lsaGenerate,NULL);
    //pthread_create(&recvThread,NULL,PacketManager::recvPacket,&inter_eth1);
    pthread_join(helloRecvThread,NULL);
    pthread_join(helloSendThread,NULL);
    pthread_join(recvThread,NULL);
    return 0;
}


int main2()
{
	uint8_t data[34]={0x02,0x01,0xc0,0xa8,0x2,0x64,0xc0,0xa8,0x02,0x64,0x80,0x00,0x00,0x11,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x01,0xc0,0xa8,0x02,0x00,0xff,0xff,0xff,0x00,0x03,0x00,0x00,0x01};
	//uint8_t data[32]={0x02,0x01,0xc0,0xa8,0x2,0x64,0xc0,0xa8,0x02,0x64,0x80,0x00,0x00,0x11,0x00,0x24,0x00,0x00,0x00,0x01,0xc0,0xa8,0x02,0x00,0xff,0xff,0xff,0x00,0x03,0x00,0x00,0x01};
	uint16_t value = Common::create_osi_cksum(data,14,34);

	printf("%x\n",value);
}