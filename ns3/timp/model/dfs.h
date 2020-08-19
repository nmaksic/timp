//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Natasa Maksic <maksicn@etf.rs>, based on OSPF Complete Implementation by John Moy
//


#ifndef DFS_H
#define DFS_H
#include "links.h"
#include <list>
#include "priq.h"

#include <map>
#include <vector>

class TLinkp;

typedef unsigned int uns32;
typedef unsigned short uns16;
typedef unsigned char byte;
typedef uns32 aid_t;
typedef uns32 rtid_t;

class Link {
public:
    Link *l_next;	
    uns16 l_fwdcst;	

};

class TNodep: public PriQElt 
{
public:
byte lsa_type;  
rtid_t adv_rtr; 
Link *t_links;  
byte t_state;	
int node_id;    
TNodep *t_parent;	
Link *lp;
TNodep *next;
TNodep(byte lsatype,rtid_t advrtr,int nodeid);    
};

typedef struct linkends {
	int switcha;
	int switchb;
	unsigned int address1;
	unsigned int address2;
	unsigned int interface1;
	unsigned int interface2;
} slinkends;

typedef struct routestruct {
	unsigned int dstaddress;
	unsigned int interface;
	unsigned int dstnode;
} sroute;

typedef struct adrstruct {
	int switch1;
	unsigned int address1;
} saddress;


class Nodes
{
	std::list<slinkends> linkends;
	bool getlinkends(int switcha, int switchb, slinkends *sle);
   std::pair <uint32_t, std::string> getifcnbr(int switcha, int switchb);
   void depthFirstSearch(std::vector<TNodep> &path, std::vector<std::vector<TNodep>> &finishedPaths, int dstid, int depth);
   int setkoeficijentidfs(std::vector<std::vector<TNodep>> &finishedPaths, std::map<unsigned int, std::list<sroute>> &routemap);
   bool checkpaths(std::vector<std::vector<TNodep>> &finishedPaths, std::vector<TNodep> &toadd);
public:
	TNodep** nodeList;
	TNodep* links;
	int nodeCount;
	int switchCount;
	void resetn();
	void addlink(int switcha, int switchb);
	void setlinkends(int switcha, int switchb, unsigned int address1, unsigned int address2, unsigned int interface1, unsigned int interface2);
	void topologyfinished();
	int getnbr(int switcha, unsigned int ifc);
	void writeroutes1(std::list<sroute> *routes, int rootst, std::map<std::pair<int,int>, int> *routingtable);
   int nDepthFirstSearch(Links* linkp);
	Nodes(int switchCount1); 
	~Nodes();
};

class TLinkp : public Link {
public:
    TNodep *tl_nbr;	
    TLinkp(TNodep *tlnbr,uns16 lfwdcst); 
};

class Graph 
{ 
    int V;    // No. of vertices 
    std::list<int> *adj;    // Pointer to an array containing adjacency lists 
    bool isCyclicUtil(int v, bool visited[], bool *rs);  // used by isCyclic() 
public: 
    Graph(int V);   // Constructor 
    void addEdge(int v, int w);   // to add an edge to graph 
    bool isCyclic();    // returns true if there is a cycle in this graph 
}; 

// Encoding of t_state

enum {
    DS_UNINIT = 0, 	// Uninitialized
    DS_ONCAND,		// On candidate list
    DS_ONTREE,		// On SPF tree
};

#endif
