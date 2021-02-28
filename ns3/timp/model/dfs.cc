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

#include "dfs.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "machdep.h"
#include <iostream>
#include <fstream>
#include <functional> 
#include <algorithm>

using namespace std;

#define ROUTEMATRICES

void Nodes::depthFirstSearch(vector<TNodep> &path, vector<vector<TNodep>> &finishedPaths, int dstid, int depth)

{
   if (depth++ > 15) {
      // stop search
      return;
   }

   TNodep &V = path.back();
   Link *lp;

   for (lp = V.t_links; lp != 0; lp = lp->l_next) { 
      TLinkp *tlp;

      tlp = (TLinkp *) lp;

      TNodep *W;

      if (!(W = tlp->tl_nbr))  
         continue;

      bool found = false;
      for (unsigned i=0; i<path.size() ; i++)
         if (W->node_id == path[i].node_id) {
            found = true;
            break;
         }
      
      if (found) 
         continue;

      if (W->node_id == dstid) {
         std::vector<TNodep> path1(path);
         path1.push_back (*W);
         finishedPaths.push_back (path1);
         continue;
      }

      bool loop = false;
      for (std::vector<TNodep>::iterator it = path.begin() ; it != path.end(); ++it)
         if (it->node_id == W->node_id)
            loop = true;

      if (!loop) {
         // recursion
         std::vector<TNodep> path1(path);
         path1.push_back (*W);
         
         /*for (std::vector<TNodep>::iterator it = path1.begin() ; it != path1.end(); ++it)
             std::cout << ' ' << it->node_id;
           std::cout << '\n';*/
         
         depthFirstSearch(path1, finishedPaths, dstid, depth);
      }

   }
}


int Nodes::nDepthFirstSearch(Links* linkp)
{
   if (linkp==NULL) 
      return(1);

   linkp->reset(); 

   std::map<unsigned int, list<sroute>> routemap;

   for(int i=0;i<nodeCount;i++) {
      if (nodeList[i]->lsa_type==1) {
         int dst=nodeList[i]->node_id;
         vector<vector<TNodep>> finishedPaths;
         finishedPaths.clear();
         std::cout << "nDepthFirstSearch dst " << dst << std::endl;
         for(int j=0;j<nodeCount;j++) { 
            if (i!=j && nodeList[i]->lsa_type==1 && nodeList[j]->lsa_type==1) {
               vector<TNodep> path;
               path.push_back(*(nodeList[j]));

               depthFirstSearch(path, finishedPaths, dst, 0); 

            }  
         }

         std::sort(finishedPaths.begin(), finishedPaths.end(), [](const vector<TNodep> & a, const vector<TNodep> & b){ return a.size() < b.size(); });

         vector<vector<TNodep>> finishedPaths1;
         finishedPaths1.clear();

         for(std::vector<vector<TNodep>>::iterator it = finishedPaths.begin(); it != finishedPaths.end(); ++it) 
            if (checkpaths(finishedPaths1, *it)) 
               finishedPaths1.push_back (*it);
   
         setkoeficijentidfs(finishedPaths1, routemap); 

      }
   }

   map<unsigned int, list<sroute>>::iterator it;
   for ( it = routemap.begin(); it != routemap.end(); it++ ) {
      std::map<std::pair<int,int>, int> routingtable;
      writeroutes1(&(it->second), it->first, &routingtable);
   }

   return(0);
}




bool Nodes::checkpaths(vector<vector<TNodep>> &finishedPaths, std::vector<TNodep> &toadd) {


   Graph g(nodeCount); 
   for (unsigned i=0; i<finishedPaths.size() ; i++) {
      vector<TNodep> &path = finishedPaths[i];
      for (unsigned j=0; j<path.size() - 1; j++) 
         g.addEdge(path[j+1].node_id, path[j].node_id);   
      
   }
   for (unsigned j=0; j<toadd.size() - 1; j++) 
      g.addEdge(toadd[j+1].node_id, toadd[j].node_id); 
   
   bool result;  
   if(g.isCyclic()) 
      result = false;
   else 
   result = true;
   
   return result;
}


int Nodes::setkoeficijentidfs(vector<vector<TNodep>> &finishedPaths, map<unsigned int, list<sroute>> &routemap)  
{                                           

   for (unsigned i=0; i<finishedPaths.size() ; i++) {
      vector<TNodep> &path = finishedPaths[i];
      int size = path.size();
      int rootst = path[size-1].node_id;
      int predzadnji = path[size-3].node_id;
      unsigned int addressr;
      slinkends sle;
      if (getlinkends(rootst, predzadnji, &sle)) {
         if (sle.switcha == rootst) {
            addressr=sle.address1;
         } else {
         if (sle.switchb == rootst) {
            addressr=sle.address2;
         } else 
            std::cout << "getlinkends error " << std::endl;
         }
      } else
         std::cout << "getlinkends failed " << rootst << " " << predzadnji << std::endl;

      sroute rt;
      rt.dstaddress = addressr;
      rt.dstnode = rootst;

      for (unsigned j=0; j<path.size()-1; j++) { 
         int tekuci=path[j].node_id; 

         if(nodeList[tekuci]->lsa_type==1) {  
            int sledeci=path[j+1].node_id; 
            if(nodeList[sledeci]->lsa_type!=1) 
               sledeci=path[j+2].node_id;  
            if (getlinkends(tekuci, sledeci, &sle)) {
                  if (sle.switcha == tekuci) {
                     rt.interface = sle.interface1;
                  } else {
                  if (sle.switchb == tekuci) {
                     rt.interface = sle.interface2;
                  } else
                     std::cout << "getlinkends error 1" << std::endl;
                  }
            } else
               std::cout << "getlinkends failed 1: " << tekuci << " " << sledeci << std::endl;

            std::map<unsigned int, list<sroute>>::iterator it = routemap.find(tekuci);
            if (it == routemap.end()) {
               list<sroute> l1;
               routemap[tekuci] = l1; // napravi pa prekopira ali nema veze
               it = routemap.find(tekuci);
            }

            list<sroute> &lst = it->second;
            auto it1 =  std::find_if(lst.begin(), lst.end(), [&rt](const sroute& element){ return element.dstaddress == rt.dstaddress && element.interface == rt.interface && element.dstnode == rt.dstnode;});
            bool found = (it1 != lst.end());

            if (!found) {
               lst.push_back (rt); 
            }
         }
      }
   }

	return 0;
}

Graph::Graph(int V) 
{ 
    this->V = V; 
    adj = new list<int>[V]; 
} 

void Graph::addEdge(int v, int w) 
{ 
    adj[v].push_back(w); // Add w to v’s list. 
} 
  
// This function is a variation of DFSUtil() in https://www.geeksforgeeks.org/archives/18212 
bool Graph::isCyclicUtil(int v, bool visited[], bool *recStack) 
{ 
    if(visited[v] == false) 
    { 
        // Mark the current node as visited and part of recursion stack 
        visited[v] = true; 
        recStack[v] = true; 
  
        // Recur for all the vertices adjacent to this vertex 
        list<int>::iterator i; 
        for(i = adj[v].begin(); i != adj[v].end(); ++i) 
        { 
            if ( !visited[*i] && isCyclicUtil(*i, visited, recStack) ) 
                return true; 
            else if (recStack[*i]) 
                return true; 
        } 
  
    } 
    recStack[v] = false;  // remove the vertex from recursion stack 
    return false; 
} 
  
// Returns true if the graph contains a cycle, else false. 
// This function is a variation of DFS() in https://www.geeksforgeeks.org/archives/18212 
bool Graph::isCyclic() 
{ 
    // Mark all the vertices as not visited and not part of recursion 
    // stack 
    bool *visited = new bool[V]; 
    bool *recStack = new bool[V]; 
    for(int i = 0; i < V; i++) 
    { 
        visited[i] = false; 
        recStack[i] = false; 
    } 
  
    // Call the recursive helper function to detect cycle in different 
    // DFS trees 
    for(int i = 0; i < V; i++) 
        if (isCyclicUtil(i, visited, recStack)) 
            return true; 
  
    return false; 
} 

void Nodes::writeroutes1(std::list<sroute> *routes, int rootst, std::map<std::pair<int,int>, int> *routingtable) {
	
   if (linkends.size() == 0)
      return;

   std::ofstream myfile;
   myfile.open ("routes1.txt", std::ios::out | std::ios::app);
   for (std::list<sroute>::iterator it = routes->begin(); it != routes->end(); it++) {
      myfile << rootst << std::endl;
      unsigned int reverse = ntohl(it->dstaddress);
      char *dot_ip = inet_ntoa(*(struct in_addr *)&(reverse));
      std::string dstaddress(dot_ip);		
      myfile << dstaddress << std::endl;
      myfile << "255.255.255.255" << std::endl;
      myfile << it->interface << std::endl;
      int nbr = getnbr(rootst, it->interface);
      myfile << nbr << std::endl;
      std::pair <uint32_t, std::string> nbrifc = getifcnbr(nbr, rootst);
      myfile << nbrifc.first << std::endl;
      myfile << nbrifc.second << std::endl;
      myfile << it->dstnode << std::endl;
      if (rootst != (int)it->dstnode)
         (*routingtable)[std::make_pair(rootst,it->dstnode)] = nbr;
   }
   myfile.close();

}



// konstruktor 

TNodep::TNodep(byte lsatype,rtid_t advrtr,int nodeid) 
{ 
   lsa_type=lsatype;
   adv_rtr=advrtr;
   node_id=nodeid;
   t_parent=NULL;
   t_links=NULL;
   lp=NULL;
   next=NULL;
}

TLinkp::TLinkp(TNodep *tlnbr,uns16 lfwdcst) 
{
   tl_nbr=tlnbr;
   l_fwdcst=lfwdcst;
   l_next=NULL;
}

void Nodes::resetn()  
{
   for(int i=0;i<nodeCount;i++) 
delete nodeList[i];
}

Nodes::~Nodes()
{
   resetn();
}

Nodes::Nodes(int switchCount1) 
{
   nodeCount = switchCount1;
   switchCount = nodeCount;

   links = NULL;

   nodeList=(TNodep**) malloc(nodeCount*sizeof(TNodep*));
	
   for (int i = 0; i < nodeCount; i++) 
	   nodeList[i] = new TNodep(1,i,i);

}

void Nodes::addlink(int switcha, int switchb) {

   int advrtr = switcha;
   if (advrtr > switchb)
      advrtr = switchb;

   int nodeid = nodeCount;
   TNodep *tmp;
   if (links != NULL) {
      TNodep *l = links;

      while (l != NULL) { 
         nodeid++;
         l=l->next;
      }		

      l = links;
      while (l->next != NULL)
         l=l->next;

      l->next = new TNodep(2,advrtr,nodeid);

      tmp = l->next;

   } else {
      links = new TNodep(2,advrtr,nodeid);
      tmp = links;
   }
   tmp->t_links = new TLinkp(nodeList[switcha], 1);
   tmp->t_links->l_next = new TLinkp(nodeList[switchb], 1);


   Link *nodelinks = (nodeList[switcha])->t_links;

   if (nodelinks != NULL) {
      while (nodelinks->l_next != NULL)
         nodelinks = nodelinks->l_next;
      nodelinks->l_next = new TLinkp(tmp, 1);
   } else
      nodeList[switcha]->t_links = new TLinkp(tmp, 1);

   nodelinks = nodeList[switchb]->t_links;
   if (nodelinks != NULL) {
      while (nodelinks->l_next != NULL)
         nodelinks = nodelinks->l_next;
      nodelinks->l_next = new TLinkp(tmp, 1);
   } else
      nodeList[switchb]->t_links = new TLinkp(tmp, 1);

}

void Nodes::setlinkends(int switcha, int switchb, unsigned int address1, unsigned int address2, unsigned int interface1, unsigned int interface2) {
   slinkends le;
   le.switcha = switcha;
   le.switchb = switchb;
   le.address1 = address1;
   le.address2 = address2;
   le.interface1 = interface1;
   le.interface2 = interface2;
   linkends.push_back(le);
}

bool Nodes::getlinkends(int switcha, int switchb, slinkends *sle) {
   for (std::list<slinkends>::iterator it = linkends.begin(); it != linkends.end(); it++) {
      if (it->switcha == switcha && it->switchb == switchb) {
         *sle = *it;			
         return true;
      }
      if (it->switcha == switchb && it->switchb == switcha) {
         sle->switcha = it->switchb;
         sle->switchb = it->switcha;
         sle->address1 = it->address2;
         sle->address2 = it->address1;
         sle->interface1 = it->interface2;
         sle->interface2 = it->interface1;
         return true;
      }
   }
   return false;
}

int Nodes::getnbr(int switcha, unsigned int ifc) {
   for (std::list<slinkends>::iterator it = linkends.begin(); it != linkends.end(); it++) {
      if (it->switcha == switcha && it->interface1 == ifc) 		
         return it->switchb;
      if (it->switchb == switcha && ifc == it->interface2) 
         return it->switcha;
   }
   return -1;
}

std::pair <uint32_t, std::string> Nodes::getifcnbr(int switcha, int switchb) {
   for (std::list<slinkends>::iterator it = linkends.begin(); it != linkends.end(); it++) {
      if (it->switcha == switcha && it->switchb == switchb) {
         unsigned int reverse = ntohl(it->address2);
         char *a1 = inet_ntoa(*(struct in_addr *)&(reverse));
         std::string ad1(a1);	
         return std::make_pair (it->interface1, ad1); // it->interface1;
      }
      if (it->switcha == switchb && it->switchb == switcha) {
         unsigned int reverse = ntohl(it->address1);
         char *a1 = inet_ntoa(*(struct in_addr *)&(reverse));
         std::string ad1(a1);	
         return std::make_pair (it->interface2, ad1);
      }
   }
   return std::make_pair (0, "0.0.0.0");
}

void Nodes::topologyfinished() {

   int brlinkova = 0;
   TNodep *l = links;

   while (l != NULL) {
      l=l->next;
      brlinkova++;		
   }

   int switchCount = nodeCount;
   nodeCount += brlinkova;

   TNodep** tmp=(TNodep**) malloc(nodeCount*sizeof(TNodep*));

   for (int i = 0; i < switchCount; i++) 
      tmp[i] = nodeList[i];	

   l = links;
   for (int i = 0; i < brlinkova; i++) {
      tmp[i+switchCount] = l;
      l=l->next;
   }

   delete nodeList;

   nodeList = tmp;

}

