#include "PacketRetransmitter.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "Interface.h"
#include "PacketManager.h"

using namespace std;

int PacketRetransmitter::registerPacket( const char *data,int len,int type,uint32_t dest_ip,int sec )
{
	packetNode node(data,len,type,dest_ip,sec);
	pthread_mutex_lock(&lock);
	node.id = currentId ++;
	
	packetList.push_back(node);
	pthread_mutex_unlock(&lock);
	return node.id;
}

bool PacketRetransmitter::unregisterPacket( int id )
{
	pthread_mutex_lock(&lock);
	for(vector<packetNode>::iterator it = packetList.begin();it!=packetList.end();it++){
		if(it->id == id){
			packetList.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&lock);
}

void* PacketRetransmitter::run( void* retrans )
{
	PacketRetransmitter* retransmitter = (PacketRetransmitter*)retrans;
	while(true){
		pthread_mutex_lock(&retransmitter->lock);
		for(int i=0;i<retransmitter->packetList.size();i++){
			packetNode* node = &retransmitter->packetList[i];
			node->count++;
			if(node->count == node->sec){
				node->count = 0;
				PacketManager::sendPacket(node->data,node->len,node->type,node->dest_ip,retransmitter->inter);
			}
		}
		pthread_mutex_unlock(&retransmitter->lock);
		sleep(1);
	}
}
