#ifndef PACKETRETRANS_H_INCLUDED
#define PACKETRETRANS_H_INCLUDED
#include <stdint.h>
#include <vector>
#include <time.h>
#include <pthread.h>

using namespace std;

class Interface;
struct packetNode{
	packetNode(const char *data,int len,int type,uint32_t dest_ip,int sec):
	data(data),sec(sec),len(len),type(type),dest_ip(dest_ip){
		count = 0;
	}

	const char *data;
	int sec;
	int count;
	int id;
	int len;
	int type;
	uint32_t dest_ip;
};

class PacketRetransmitter{
public:
	PacketRetransmitter(Interface* inter):currentId(0),inter(inter){
		pthread_mutex_init(&lock,NULL);
	}

	pthread_mutex_t lock;
	static void* run(void* retrans);
	Interface* inter;
	int currentId;
	vector<packetNode> packetList;
	int registerPacket(const char *data,int len,int type,uint32_t dest_ip,int sec);
	bool unregisterPacket(int id);
	vector<timer_t> timers;
};

#endif