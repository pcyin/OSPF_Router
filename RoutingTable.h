#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H
#include <sys/types.h>
#include <map>
#include <set>
#include <unistd.h>
#include <stdint.h>
using namespace std;
class Interface;
struct hop{
	Interface* inter;
	uint32_t ip;
	bool operator<(const hop& other) const {
		return ip < other.ip;
	}
};
struct vertex{
	uint32_t id;
	uint32_t pid;
	int type;
	int cost;
	set<hop> nextHop;
};
class RoutingTable
{
public:
	RoutingTable(void);
	~RoutingTable(void);
	void calc();
	map<uint32_t,vertex> clist;
	map<uint32_t,vertex> tree;
	void getNextHop(vertex& node,set<hop>& nextHop);
};
#endif
