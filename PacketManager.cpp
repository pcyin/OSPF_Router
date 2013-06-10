#include "PacketManager.h"
#include <unistd.h>
#include <iostream>
#define IP_LEN (20)
#include "Config.h"
#include "Interface.h"
#include "Neighbor.h"
#include "NeighborEventState.h"
#include "InterfaceEventState.h"
#include "LSDatabase.h"

PacketManager::PacketManager()
{
    //ctor
}

int inet_cksum(unsigned short *data,int length)
{
    register int                nleft   = length;
    register unsigned short     *w   = data;
    register int                sum = 0;
    unsigned short              answer  = 0;

    while (nleft > 1)
    {
        sum     += *w++;
        nleft   -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum+=answer;
    }

    sum = (sum>>16) + (sum & 0xffff);
    sum += (sum>>16);
    answer = ~sum;

    return answer;
}

void *PacketManager::recvPacket(void *inter){
    Interface* interface = (Interface*)inter;
    int fd;
    char msgbuf[1024];
    if((fd = socket(AF_INET,SOCK_RAW,htons(89)))<0){
        perror("recvPacket");
    }
    printf("%d\n",fd);

    struct ifreq eth1;

    bzero(&eth1,sizeof(eth1));
    snprintf(eth1.ifr_name, sizeof(eth1.ifr_name), "eth1");

   // if(setsockopt(fd,SOL_SOCKET,SO_BINDTODEVICE,(void*)&eth1,sizeof(eth1))<0)
   // {
    //    perror("recvPacket");
    //}

    while(true){
        bzero(msgbuf,sizeof(msgbuf));
        recv(fd,msgbuf,sizeof(msgbuf),0);
        struct ospf_hdr* hdr = (struct ospf_hdr*)(msgbuf+IP_LEN);

        hdr->packet_len = ntohs(hdr->packet_len);
        hdr->router_id = ntohl(hdr->router_id);
        hdr->area_id = ntohl(hdr->area_id);
        hdr->checksum = ntohl(hdr->checksum);
        in_addr_t sourceIp = ntohl(*(uint32_t*)(msgbuf+12));
        if(sourceIp == interface->ip)
            continue;

        if(hdr->type == 2){
            printf("dd\n");
            Neighbor* nbr;
            bool accepted = false;
            ospf_dd *dd = (ospf_dd*)(msgbuf+IP_LEN+HEADER_LEN);
            int seq_num = ntohl(dd->seq_num);
            nbr = interface->getNbrByIp(sourceIp);
            if(nbr->currentState == NBR_ST_INIT){
                EventArgs arg;
                arg.eventId = NBR_EVT_2WAYRECV;
                nbr->invokeEvent(PACKET_RECV,arg);
            }
            if(nbr->currentState == NBR_ST_EXSTART){
                if(dd->mbit && dd->ibit && dd->msbit && nbr->id > Config::routerId){
                    nbr->isMaster = true;
                    nbr->ddNum = seq_num;
                }else if(!dd->ibit && !dd->msbit && seq_num == nbr->ddNum && nbr->id < Config::routerId ){
                    nbr -> isMaster = false;
                }else
                    continue;
                EventArgs arg;
                arg.eventId = NBR_EVT_NEGOTIATIONDONE;
                nbr->invokeEvent(PACKET_RECV,arg);
            }
            if(nbr->currentState == NBR_ST_EXCHANGE){
                if(nbr->isMaster && seq_num == nbr->ddNum +1 || !nbr->isMaster && seq_num == nbr->ddNum)
                    accepted = true;
            }
            if(accepted){
                ospf_lsa_header *lsaPtr = (ospf_lsa_header*)(msgbuf+IP_LEN+DD_HEADER_LEN);
                for(;lsaPtr!=(ospf_lsa_header *)(msgbuf+hdr->packet_len);lsaPtr++){
                    ospf_lsa_header lsa;
                    lsa.ad_router = ntohl(lsaPtr->ad_router);
                    lsa.state_id=ntohl(lsaPtr->state_id);
                    lsa.seq_num=ntohl(lsaPtr->seq_num);
                    lsa.type=lsaPtr->type;
                }

                if(nbr->isMaster){
                    nbr->ddNum = seq_num;
                    ospf_dd ddAck;
                    ddAck.mbit = dd->mbit;
                    ddAck.seq_num = htonl(seq_num);
                    sendPacket((const char *)&ddAck,DD_HEADER_LEN,2,nbr->ip,nbr->interface);
                    if(dd->mbit == 0){
                        EventArgs arg;
                        arg.eventId = NBR_EVT_EXCHANGEDONE;
                        nbr->invokeEvent(PACKET_RECV,arg);
                    }

                }else{
                    nbr->ddNum++;
                }
            }
        }
    }

}

void* PacketManager::recvHelloPacket(void *inter)
{
    Interface* interface = (Interface*)inter;

    int ospfsocket=socket(AF_INET,SOCK_RAW,89);

    printf("%d\n",ospfsocket);


    struct sockaddr_in inaddr;
    bzero((void *)&inaddr,sizeof(inaddr));
    inaddr.sin_family=AF_INET;
    inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    char msgbuf[1024];

    struct ip_mreq mreq;
    bzero(&mreq,sizeof(mreq));
    mreq.imr_multiaddr.s_addr=inet_addr("224.0.0.5");
    mreq.imr_interface.s_addr=htonl(interface->ip);

    if (setsockopt(ospfsocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    while(1)
    {
        bzero(msgbuf,sizeof(msgbuf));
        recv(ospfsocket,msgbuf,sizeof(msgbuf),0);
        struct ospf_hdr* hdr = (struct ospf_hdr*)(msgbuf+IP_LEN);

        hdr->packet_len = ntohs(hdr->packet_len);
        hdr->router_id = ntohl(hdr->router_id);
        hdr->area_id = ntohl(hdr->area_id);
        hdr->checksum = ntohl(hdr->checksum);
        in_addr_t sourceIp = ntohl(*(uint32_t*)(msgbuf+12));
        if(sourceIp == interface->ip)
            continue;

        if(hdr->type==1)
        {
            Neighbor* nbr;

            ospf_hello *hello = (ospf_hello*)(msgbuf+20+sizeof(ospf_hdr));

            if((nbr = interface->getNbrByIp(sourceIp))==NULL)
            {
                nbr = interface->createNbr(sourceIp);
            }

            nbr->id = hdr->router_id;
            int pndr = nbr->ndr;
            int pnbdr = nbr->nbdr;
            bool twoWay = false;

            nbr->ndr = ntohl(hello->dr);
            nbr->nbdr = ntohl(hello->bdr);

            EventArgs arg;
            arg.eventId = NBR_EVT_HELLORECV;
            nbr->invokeEvent(PACKET_RECV,arg);

            uint32_t *endPtr = (uint32_t*)(msgbuf+hdr->packet_len+IP_LEN);
            int helloLen = IP_LEN+sizeof(ospf_hdr)+sizeof(ospf_hello);
            for(uint32_t *ptr = (uint32_t*)(msgbuf+helloLen); ptr!=endPtr; ptr++)
            {
                if(*ptr == htonl(Config::routerId))
                {
                    EventArgs args;
                    args.eventId = NBR_EVT_2WAYRECV;
                    twoWay = true;
                    nbr->invokeEvent(PACKET_RECV,args);
                }
            }

            if(!twoWay){
                EventArgs args;
                args.eventId = NBR_EVT_1WAY;
                nbr->invokeEvent(PACKET_RECV,args);
                continue;
            }

            if(nbr->ip == nbr->ndr && nbr->nbdr == 0x00000000 && interface->currentState == IT_ST_WAITING)
            {
                EventArgs args;
                args.eventId = IT_EVT_BACKUPSEEN;
                interface->invokeEvent(PACKET_RECV,args);
            }else if(nbr->ip == nbr->ndr && pndr != nbr->ip || nbr->ip != nbr->ndr && pndr == nbr->ip){
                EventArgs args;
                args.eventId = IT_EVT_NEIGHBORCHANGE;
                interface->invokeEvent(PACKET_RECV,args);
            }

            if(nbr->ip == nbr->nbdr && interface->currentState == IT_ST_WAITING)
            {
                EventArgs args;
                args.eventId = IT_EVT_BACKUPSEEN;
                interface->invokeEvent(PACKET_RECV,args);
            }else if(nbr->ip == nbr->nbdr && pnbdr != nbr->ip || nbr->ip != nbr->nbdr && pnbdr == nbr->ip){
                EventArgs args;
                args.eventId = IT_EVT_NEIGHBORCHANGE;
                interface->invokeEvent(PACKET_RECV,args);
            }

        }else if(hdr->type == 2){
            printf("dd recved\n");
            Neighbor* nbr;
            bool accepted = false;
            ospf_dd *dd = (ospf_dd*)(msgbuf+IP_LEN+HEADER_LEN);
			
            int seq_num = ntohl(dd->seq_num);
            nbr = interface->getNbrByIp(sourceIp);
			bool isDup = false;
			if(nbr->last_seq_num == seq_num){
				isDup = true;
				cout << "dup dd packet!" << endl;
			}else{
				nbr->last_seq_num = seq_num;
			}

            if(nbr->currentState == NBR_ST_INIT){
                EventArgs arg;
                arg.eventId = NBR_EVT_2WAYRECV;
                nbr->invokeEvent(PACKET_RECV,arg);
            }
            if(nbr->currentState == NBR_ST_EXSTART){
                if(dd->mbit && dd->ibit && dd->msbit && nbr->id > Config::routerId){
                    nbr->isMaster = true;
                    nbr->ddNum = seq_num;
                    seq_num ++;
                }else if(!dd->ibit && !dd->msbit && seq_num == nbr->ddNum && nbr->id < Config::routerId ){
                    nbr -> isMaster = false;
                }else
                    continue;
                EventArgs arg;
                arg.eventId = NBR_EVT_NEGOTIATIONDONE;
                nbr->invokeEvent(PACKET_RECV,arg);
            }
            if(nbr->currentState == NBR_ST_EXCHANGE){
				if(isDup){
					if(nbr->isMaster){
						cout << "dup dd packet"<<endl;
						sendPacket(nbr->lastDD,nbr->lastDD_len,2,nbr->ip,nbr->interface);
					}
				}

                if(nbr->isMaster && seq_num == nbr->ddNum +1 || !nbr->isMaster && seq_num == nbr->ddNum)
                    accepted = true;
			}else if(nbr->currentState >= NBR_ST_LOADING){
				if(isDup){
					if(nbr->isMaster){
						cout << "dup dd packet"<<endl;
						sendPacket(nbr->lastDD,nbr->lastDD_len,2,nbr->ip,nbr->interface);
					}
				}
			}
            if(accepted){
                seq_num = ntohl(dd->seq_num);
                printf("dd packet is accepted\n");
                ospf_lsa_header *lsaPtr = (ospf_lsa_header*)(msgbuf+IP_LEN+HEADER_LEN+DD_HEADER_LEN);
                for(;lsaPtr!=(ospf_lsa_header *)(msgbuf+IP_LEN+hdr->packet_len);lsaPtr++){
                    ospf_lsa_header lsa;
                    lsa.ad_router = ntohl(lsaPtr->ad_router);
                    lsa.state_id=ntohl(lsaPtr->state_id);
                    //lsa.seq_num=ntohl(lsaPtr->seq_num);
                    lsa.seq_num = 9;
                    lsa.type=lsaPtr->type;

                    if(lsa.type == 1){
                        cout << "router lsa recved" << endl;
                        RouterLSA *rlsa = LSDatabase::getRouterLSAById(lsa.state_id,lsa.ad_router);
                        if(rlsa == NULL){
                            nbr->reqList.push_back(lsa);
                        }
                    }else if(lsa.type==2){
                        cout << "network lsa recved" << endl;
                        NetworkLSA *nlsa = LSDatabase::getNetworkLSAById(lsa.state_id,lsa.ad_router);
                        if(nlsa == NULL){
                            nbr->reqList.push_back(lsa);
                        }
                    }
                }

                if(nbr->isMaster){
                    nbr->ddNum = seq_num;
                    ospf_dd ddAck;
                    bzero(&ddAck,DD_HEADER_LEN);
                    if(dd->mbit == 0)
                        ddAck.mbit = 0;
                    else
                        ddAck.mbit = 0;
                    ddAck.seq_num = htonl(seq_num);
                    ddAck.interMTU = htons(nbr->interface->mtu);
                    ddAck.opt = 0x02;

					ospf_lsa_header lsa = *nbr->summaryList.begin();
					char data[1024];
					memcpy(data,&ddAck,DD_HEADER_LEN);
					int packetLen = DD_HEADER_LEN;
					if(nbr->summaryList.size() > 0){
						nbr->summaryList.pop_front();
						lsa.ad_router = htonl(lsa.ad_router);
						lsa.checksum = htons(lsa.checksum);
						lsa.len = htons(lsa.len);
						lsa.ls_age = htons(lsa.ls_age);
						lsa.seq_num = htonl(lsa.seq_num);
						lsa.state_id = htonl(lsa.state_id);

						memcpy(data+packetLen,&lsa,LSA_HEADER_LEN);
						packetLen+=LSA_HEADER_LEN;
					}

                    sendPacket(data,packetLen,2,nbr->ip,nbr->interface);
					memcpy(nbr->lastDD,data,packetLen);
					nbr->lastDD_len = packetLen;

                    if(dd->mbit == 0){
                        EventArgs arg;
                        arg.eventId = NBR_EVT_EXCHANGEDONE;
                        nbr->invokeEvent(PACKET_RECV,arg);
                    }

                }else{
                    nbr->ddNum++;
                }
            }
		}else if(hdr->type == 3){
			cout<<"LS Req recved"<<endl;
			ospf_lsreq *ptr = (ospf_lsreq*)(msgbuf+IP_LEN+HEADER_LEN);
			ospf_lsreq *lastPtr = (ospf_lsreq*)(msgbuf+IP_LEN+hdr->packet_len);
			int num=0;
			int dataPtr = 4;
			char data[MAX_PACKET_LEN];
			for( ; ptr!=lastPtr ; ptr++ ){
				ospf_lsreq lsreq;
				num++;

				lsreq.ad_router = ntohl(ptr->ad_router);
				lsreq.type = ntohl(ptr->type);
				lsreq.state_id = ntohl(ptr->state_id);

				if(lsreq.type == 1){
					RouterLSA* rlsa = LSDatabase::getRouterLSAById(lsreq.state_id,lsreq.ad_router);
					int len = rlsa->size();
					memcpy(data+dataPtr,rlsa->toRouterLSA(),len);
					dataPtr+=len;
				}else if(lsreq.type == 2){
					NetworkLSA* nlsa = LSDatabase::getNetworkLSAById(lsreq.state_id,lsreq.ad_router);
					int len = nlsa->size();
					memcpy(data+dataPtr,nlsa->toNetworkLSA(),len);
					dataPtr+=len;
				}
			}
			*(uint32_t*)data = htonl(num);
			sendPacket(data,dataPtr,4,sourceIp,interface);
		}else if(hdr->type==4){
			cout<<"LS Update recved"<<endl;
			ospf_lsupdate *lsupdate = (ospf_lsupdate*)(msgbuf+IP_LEN+HEADER_LEN);
			int num = ntohl(lsupdate->num);
			ospf_lsa_header* hdr = (ospf_lsa_header*)((uint32_t*)lsupdate+1);
			ospf_lsa_header* hdr_host = hdr->toHostOrder();
			for(int i=0;i<num;i++){
				switch (hdr->type)
				{
				case 1:{
					RouterLSA *lsa = LSDatabase::getRouterLSAById(hdr_host->state_id,hdr_host->ad_router);
					if(lsa == NULL){
						RouterLSA rlsa(*hdr_host);
						LSDatabase::insertRouterLSA(rlsa);
					}
					else if(*hdr_host > lsa->header){
						LSDatabase::remove(lsa->header.state_id,lsa->header.ad_router,1);
						RouterLSA rlsa(*hdr_host);
						LSDatabase::insertRouterLSA(rlsa);
					}
				}	
					break;
				case 2:{
					NetworkLSA *lsa = LSDatabase::getNetworkLSAById(hdr_host->state_id,hdr_host->ad_router);
					if(lsa == NULL){
						NetworkLSA nlsa(*hdr_host);
						LSDatabase::insertNetworkLSA(nlsa);
					}
					else if(*hdr_host > lsa->header){
						LSDatabase::remove(hdr_host->state_id,lsa->header.ad_router,2);
						NetworkLSA nlsa(*hdr_host);
						LSDatabase::insertNetworkLSA(nlsa);
					}
				}	
					break;
				default:
					break;
				}
				sendPacket((char *)hdr,LSA_HEADER_LEN,5,ntohl(inet_addr("224.0.0.5")),interface);
			}
		}
    }
    return 0;
}

void *PacketManager::sendLSReq(void* nbr){
    Neighbor *neighbor = (Neighbor*)nbr;

    for(list<ospf_lsa_header>::iterator it = neighbor->reqList.begin() ; it != neighbor->reqList.end() ; ){
        ospf_lsreq req;
        req.ad_router = htonl(it->ad_router);
        req.state_id = htonl(it->state_id);
        req.type = htonl(it->type);
        sendPacket((const char*)&req,LSREQ_LEN,3,neighbor->ip,neighbor->interface);
        it = neighbor->reqList.erase(it);
        cout << "send a lsreq packet!" << endl;
    }
}

void* PacketManager::sendHello(void* inter)
{
    Interface* interface = (Interface*)inter;
    struct ospf_hdr *hdr;
    struct sockaddr_in sockdst;
    int ospfsocket=socket(AF_INET,SOCK_RAW,89);
    char *ospf_sendbuf=(char *)malloc(64*1024);
    struct ifreq eth1;

    bzero(&eth1,sizeof(eth1));
    snprintf(eth1.ifr_name, sizeof(eth1.ifr_name), "eth1");

    if(setsockopt(ospfsocket,SOL_SOCKET,SO_BINDTODEVICE,(void*)&eth1,sizeof(eth1))==0)
    {
        printf("hello packet sending thread setsockopt success!\n");
    }

    while(1)
    {
        hdr = (struct ospf_hdr*)ospf_sendbuf;
        hdr->version = 2;
        hdr->type = 1;
        hdr->packet_len = 0;
        hdr->checksum=0;
        hdr->router_id = htonl(Config::routerId);
        hdr->area_id = htonl(interface->areaId);
        hdr->autype = 0;
        hdr->auth = 0;

        struct ospf_hello *hello = (struct ospf_hello*)(ospf_sendbuf+sizeof(ospf_hdr));
        hello->nmask = htonl(interface->mask);//inet_addr("255.255.255.0");
        hello->opt=0x02;
        hello->interval=htons(10);
        hello->rtr_pri=1;
        hello->router_dead_interval=htonl(40);
        hello->dr=htonl(interface->dr);
        hello->bdr=htonl(interface->bdr);

        int len = sizeof(ospf_hdr)+sizeof(ospf_hello);

        int actNbrNum = 0;
        uint32_t *ptr = (uint32_t *)(ospf_sendbuf+len);

        for(list<Neighbor*>::iterator it = interface->nbrList.begin(); it!=interface->nbrList.end(); it++)
        {
            actNbrNum++;
            *ptr = htonl((*it)->ip);
            ptr++;
        }
        cout<<"actNum:"<<actNbrNum<<endl;
        int totalLen = len+actNbrNum*4;
        hdr->packet_len = htons(totalLen);
        hdr->checksum = inet_cksum((unsigned short*)hdr,totalLen);

        bzero((void *)&sockdst,sizeof(sockdst));
        sockdst.sin_family=AF_INET;
        sockdst.sin_port=sizeof(sockdst);
        sockdst.sin_addr.s_addr=inet_addr("224.0.0.5");

        sendto(ospfsocket,ospf_sendbuf,totalLen,0,(struct sockaddr *)&sockdst,sizeof(sockdst));
        sleep(10);
    }
}

void *PacketManager::sendEmptyDDPacets(void *nbr){
    Neighbor* neighbor = (Neighbor*)(nbr);
	while(true){
		if(neighbor->currentState != NBR_ST_EXSTART){
			break;
		}
        sendDDPackets(nbr);
        sleep(5);
    }
}

void *PacketManager::sendDDPackets(void *nbr){
    Neighbor *neighbor = (Neighbor*)(nbr);
    bool isEmpty = false;
    if(neighbor->currentState == NBR_ST_EXSTART)
        isEmpty = true;

    ospf_dd packet;
    bzero(&packet,sizeof(packet));
    packet.interMTU = htons(neighbor->interface->mtu);
    packet.opt = 0x02;
    packet.seq_num = neighbor->ddNum;
    if(isEmpty){
        packet.ibit = 1;
        packet.mbit = 1;
        packet.msbit = 1;
    }

    sendPacket((const char*)&packet,sizeof(packet),2,neighbor->ip,neighbor->interface);
}

void PacketManager::sendPacket(const char *packet,int len,int type,uint32_t ip,Interface* inter){
        struct ospf_hdr *hdr;
        struct sockaddr_in sockdst;
        int ospfsocket=socket(AF_INET,SOCK_RAW,89);
        char *sendbuf=(char *)malloc(64*1024);
        struct ifreq eth1;

        bzero(&eth1,sizeof(eth1));
        snprintf(eth1.ifr_name, sizeof(eth1.ifr_name), "eth1");

        if(setsockopt(ospfsocket,SOL_SOCKET,SO_BINDTODEVICE,(void*)&eth1,sizeof(eth1))<0)
        {
            perror("setsockopt in sendPacket");
        }

        hdr = (struct ospf_hdr*)sendbuf;
        hdr->version = 2;
        hdr->type = type;
        hdr->packet_len = 0;
        hdr->checksum=0;
        hdr->router_id = htonl(Config::routerId);
        hdr->area_id = htonl(((Interface*)inter)->areaId);
        hdr->autype = 0;
        hdr->auth = 0;

        memcpy(sendbuf+HEADER_LEN,packet,len);
        int totalLen = HEADER_LEN + len;

        hdr->packet_len = htons(totalLen);
        hdr->checksum = inet_cksum((unsigned short*)sendbuf,totalLen);

        bzero((void *)&sockdst,sizeof(sockdst));
        sockdst.sin_family=AF_INET;
        sockdst.sin_port=sizeof(sockdst);
        sockdst.sin_addr.s_addr=htonl(ip);

        if(sendto(ospfsocket,sendbuf,totalLen,0,(struct sockaddr *)&sockdst,sizeof(sockdst)) < 0){
            perror("sendto in sendPacket");
        }
}
