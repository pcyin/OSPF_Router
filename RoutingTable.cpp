#include "RoutingTable.h"
#include "LSDatabase.h"
#include "Config.h"
#include <vector>
#include "Interface.h"
#include "Neighbor.h"

RoutingTable::RoutingTable(void)
{
}


RoutingTable::~RoutingTable(void)
{
}

void RoutingTable::calc()
{
	clist.clear();
	tree.clear();
	vertex root;
	root.cost = 0;
	root.id = Config::routerId;
	root.type=1;
	root.pid==0;
	tree[root.id] = root;
	vertex* newNode = &tree[root.id];
	while(true){
		//router
		if(newNode->type == 1){
			RouterLSA *lsa = LSDatabase::getRouterLSAByLinkStateId(newNode->id);
			for(vector<RouterLSALink>::iterator it = lsa->links.begin();it!=lsa->links.end();it++){
				if(it->type == 3)
					continue;
				vertex node;
				switch (it->type)
				{
				//transit network
				case 2:
					{
						if(LSDatabase::getNetworkLSAByLinkStateId(it->linkId)==NULL || tree.count(it->linkId)>0){
							continue;
						}
						node.type=2;
					}
				//router point-to-point
				case 1:
					{
						if(LSDatabase::getRouterLSAByLinkStateId(it->linkId)==NULL || tree.count(it->linkId)>0){
							continue;
						}
						node.type = 1;
					}
				default:
					break;
				}

				node.id=it->linkId;
				node.cost = newNode->cost + it->metric;
				node.pid = newNode->id;
				if(clist.count(node.id)==0 || clist[node.id].cost > node.cost){
					getNextHop(node,node.nextHop);
					clist[node.id] = node;
				}else if(clist[node.id].cost == node.cost){
					getNextHop(node,node.nextHop);
					for(set<hop>::iterator it = node.nextHop.begin();it!=node.nextHop.end();it++){
						clist[node.id].nextHop.insert(*it);
					}
				}
			}	
		}
		//transit network
		else{
			NetworkLSA *lsa = LSDatabase::getNetworkLSAByLinkStateId(newNode->id);
			vertex node;
			for(vector<uint32_t>::iterator it = lsa->routers.begin();it!=lsa->routers.end();it++){
				if(LSDatabase::getRouterLSAByLinkStateId(*it)==NULL || tree.count(*it))
					continue;
				node.cost = newNode->cost;
				node.id = *it;
				node.pid = newNode->id;
				node.type = 1;

				if(clist.count(node.id)==0 || clist[node.id].cost > node.cost){
					getNextHop(node,node.nextHop);
					clist[node.id] = node;
				}else if(clist[node.id].cost == node.cost){
					getNextHop(node,node.nextHop);
					for(set<hop>::iterator it = node.nextHop.begin();it!=node.nextHop.end();it++){
						clist[node.id].nextHop.insert(*it);
					}
				}
			}
		}

		//find out vertex in clist with shortest dist from root vertex
		uint32_t cost = 0xffffffff;
		uint32_t id = 0;
		for(map<uint32_t,vertex>::iterator it = clist.begin();it!=clist.end();it++){
			if(it->second.cost<=cost){
				if(id!=0 && clist[id].type == 2 && it->second.type==1 && cost == it->second.cost)
					continue;

				cost = it->second.cost;
				id = it->first;
			}
		}
		if(id!=0){
			tree[id] = clist[id];
			clist.erase(id);
			newNode = &tree[id];
		}
	}
}

void RoutingTable::getNextHop( vertex& node,set<hop>& nextHop )
{
	bool intervene = false;
	uint32_t ptr = node.pid;
	while((ptr = tree[ptr].pid)!=0){
		if(tree[ptr].type==1){
			intervene = true;
			break;
		}
	}
	if(intervene){
		nextHop = clist[node.pid].nextHop;
	}
	else{
		hop hopnode;
		//parent is the calc router
		if(tree[node.pid].type==1){
			for(vector<Interface*>::iterator it = Config::inters.begin();it!=Config::inters.end();it++){
				if(node.type==1){
					for(list<Neighbor*>::iterator nit = (*it)->nbrList.begin();nit!=(*it)->nbrList.end();nit++)
					{
						if((*nit)->id == node.id){
							hopnode.inter = *it;
							
						}
					}
				}
				//transit network
				else{
					if((*it)->getNbrByIp((*it)->dr)->id == node.id){
						hopnode.inter = *it;
						hopnode.ip = 0;
					}
				}
			}
		}
		//parent is transit network
		else{
			RouterLSA* lsa = LSDatabase::getRouterLSAByLinkStateId(node.id);
			for(vector<RouterLSALink>::iterator it = lsa->links.begin();it!=lsa->links.end();it++){
				if(it->linkId == tree[node.pid].id){
					hopnode.ip = it->linkData;
					for(vector<Interface*>::iterator iit = Config::inters.begin();iit!=Config::inters.end();iit++){
						if((*iit)->getNbrByIp(hopnode.ip)!=NULL){
							hopnode.inter = *iit;
							break;
						}	
					}
					break;
				}
			}
		}
		nextHop.insert(hopnode);
	}
}
