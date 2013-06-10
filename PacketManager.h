#ifndef PACKETMANAGER_H
#define PACKETMANAGER_H

#include "Packets.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<strings.h>
#include<string.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

class Interface;
class Neighbor;
class PacketManager
{
public:
    PacketManager();
    static void* recvPacket(void* interface);
    //static void* sendPacket(void* interface);
    static void* sendHello(void* inter);
    static void* sendDDPackets(void *nbr);
    static void sendPacket(const char*,int,int,uint32_t,Interface*);
    static void* recvHelloPacket(void *inter);
    static void *sendEmptyDDPacets(void *nbr);
    static void *sendLSReq(void* nbr);
protected:
private:
};

#endif // PACKETMANAGER_H
