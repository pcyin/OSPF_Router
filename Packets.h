#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include <vector>
#include "Common.h"
#define HEADER_LEN sizeof(ospf_hdr)
#define DD_HEADER_LEN sizeof(ospf_dd)
#define LSA_HEADER_LEN sizeof(ospf_lsa_header)
#define LSREQ_LEN sizeof(ospf_lsreq)
#define MAX_PACKET_LEN 1024

using namespace std;

struct ospf_hdr{
    uint8_t version;
    uint8_t type;
    uint16_t packet_len;
    uint32_t router_id;
    uint32_t area_id;
    uint16_t checksum;
    uint16_t autype;
    uint64_t auth;
};

struct ospf_hello{
    uint32_t nmask;
    uint16_t interval;
    uint8_t opt;
    uint8_t rtr_pri;
    uint32_t router_dead_interval;
    uint32_t dr;
    uint32_t bdr;
};

struct ospf_dd{
    uint16_t interMTU;
    uint8_t opt;
    uint8_t msbit: 1;
    uint8_t mbit:1;
    uint8_t ibit: 1;
    uint8_t zeros: 5;
    uint32_t seq_num;
};

struct ospf_lsa_header{
	ospf_lsa_header():
	ls_age(100),seq_num(0x80000000),opt(2),checksum(0){}

    uint16_t ls_age;
    uint8_t opt;
    uint8_t type;
    uint32_t state_id;
    uint32_t ad_router;
    int seq_num;
    uint16_t checksum;
    uint16_t len;

	ospf_lsa_header *toNetworkOrder(){
		ospf_lsa_header *hdr = new ospf_lsa_header(*this);
		hdr->ls_age = htons(ls_age);
		hdr->state_id = htonl(state_id);
		hdr->ad_router = htonl(ad_router);
		hdr->checksum = htons(checksum);
		hdr->len = htons(len);
		hdr->seq_num = htonl(seq_num);
		return hdr;
	}

	bool operator>(const ospf_lsa_header& s)
	{
		return seq_num > s.seq_num;
	}

	ospf_lsa_header *toHostOrder()
	{
		ospf_lsa_header *hdr = new ospf_lsa_header(*this);
		hdr->ls_age = ntohs(ls_age);
		hdr->state_id = ntohl(state_id);
		hdr->ad_router = ntohl(ad_router);
		hdr->checksum = ntohs(checksum);
		hdr->len = ntohs(len);
		hdr->seq_num = ntohl(seq_num);
		return hdr;
	}

};

struct RouterLSALink{
    RouterLSALink():tosNum(0){}

    uint32_t linkId;
    uint32_t linkData;
    uint8_t type;
    uint8_t tosNum;
    uint16_t metric;

    bool operator==(const RouterLSALink& l){
        return linkId == l.linkId && linkData == l.linkData && type == l.type && tosNum == l.tosNum && metric == l.metric;
    }

    bool operator!=(const RouterLSALink& l){
        return (! (*this == l));
    }
};

struct RouterLSA{
    RouterLSA(ospf_lsa_header hdr):
        zeros(0),linkNum(0),header(hdr){}
	RouterLSA():zeros(0),linkNum(0){}

    uint8_t *toRouterLSA(){
        uint32_t *data = new uint32_t[this->size()/4];
		refresh();
		header.checksum = 0;
		memcpy(data,header.toNetworkOrder(),LSA_HEADER_LEN);
        *(uint16_t*)&data[5] = zeros;
		*((uint16_t*)&data[5] + 1) = htons(linkNum);
        for(int i=0;i<links.size();i++){
            data[6+i*3] = htonl(links[i].linkId);
            data[6+i*3+1] = htonl(links[i].linkData);
			*(uint8_t*)&data[6+i*3+2] = links[i].type;
			*((uint8_t*)&data[6+i*3+2]+1) = links[i].tosNum;
			*((uint16_t*)&data[6+i*3+2]+1) = htons(links[i].metric);
        }
		header.checksum = Common::create_osi_cksum((uint8_t*)data + 2,14,header.len - 2);
		((ospf_lsa_header*)data)->checksum = htons(header.checksum);
        return (uint8_t*)data;
    }

	int size(){
		return (LSA_HEADER_LEN + 4 +12*links.size());
	}

	void refresh(){
		linkNum = links.size();
		header.len = size();
	}

    ospf_lsa_header header;
    uint16_t zeros;
    uint16_t linkNum;
    vector<RouterLSALink> links;

    bool operator==(const RouterLSA& s)
    {
        if(header.len != s.header.len || s.header.state_id != header.state_id || s.header.ad_router != header.ad_router)
            return false;
        if(s.linkNum != linkNum)
            return false;
        for(int i=0;i<linkNum;i++){
            if(links[i] != s.links[i])
                return false;
        }
        return true;
    }

	bool operator>(const RouterLSA& s)
	{
		return header.seq_num > s.header.seq_num;
	}

    bool operator!=(const RouterLSA& s){
        return !( (*this) == s );
    }
};

struct NetworkLSA{
    NetworkLSA():
        netMask(0xffffff00){}
	NetworkLSA(ospf_lsa_header hdr):
		netMask(0xffffff00),header(hdr){}

    ospf_lsa_header header;
    uint32_t netMask;
    vector<uint32_t> routers;

	int size(){
		return  LSA_HEADER_LEN + 4 + 4*routers.size();
	}

	uint8_t *toNetworkLSA(){
		uint8_t *data = new uint8_t[size()];
		header.checksum = 0;
		memcpy(data,header.toNetworkOrder(),LSA_HEADER_LEN);
		uint32_t *ptr = (uint32_t *)(data + LSA_HEADER_LEN);
		for(int i = 0;i<routers.size();i++){
			ptr[i] = htonl(routers[i]);
		}
		return data;
	}

    bool operator==(const NetworkLSA& s)
    {
        if(header.len != s.header.len || s.header.state_id != header.state_id || s.header.ad_router != header.ad_router)
            return false;
        if(s.netMask != netMask)
            return false;
        if(s.routers.size()!=routers.size())
            return false;
        for(int i=0;i<routers.size();i++){
            if(routers[i] != s.routers[i])
                return false;
        }
        return true;
    }

    bool operator!=(const NetworkLSA& s){
        return !( (*this) == s );
    }
};

struct ospf_lsreq{
    uint32_t type;
    uint32_t state_id;
    uint32_t ad_router;
};

struct ospf_lsupdate{
	uint32_t num;
};

struct ospf_lsack{
	list<ospf_lsa_header> ackHdrList;
	uint8_t* toPacketData(){
		uint8_t *data = new uint8_t[ackHdrList.size()*LSA_HEADER_LEN];
		int i=0;
		for(list<ospf_lsa_header>::iterator it = ackHdrList.begin();it!=ackHdrList.end();it++,i+=LSA_HEADER_LEN){
			memcpy(data+i,it->toNetworkOrder(),LSA_HEADER_LEN);
		}
		return data;
	}
};
#endif // PACKETS_H_INCLUDED
