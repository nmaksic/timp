/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Natasa Maksic <maksicn@etf.rs>
 *
 */

#include "links.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

Edge::Edge(int nodeCount) {
	coefficientsDijkstra=(unsigned char **) malloc(nodeCount*sizeof(unsigned char*));
	for(int i=0;i<nodeCount;i++)
		coefficientsDijkstra[i]=(unsigned char *) malloc(nodeCount*sizeof(unsigned char));
	this->nodeCount=nodeCount;
	resetCoefficients();
}

Edge::~Edge() {
	for(int i=0;i<nodeCount;i++)
		free(coefficientsDijkstra[i]);
	free(coefficientsDijkstra);
}

int Edge::setNewCoefficient(int src,int dest) {
	if ((src<0) || (src>nodeCount) || (dest<0) || (dest>nodeCount)) {
		printf("setNewCoefficient bad param\n");	
		return -1;
	}
	coefficientsDijkstra[src][dest]=1;
	return 0;
}

void Edge::resetCoefficients() {
	for(int i=0;i<nodeCount;i++)
		memset(coefficientsDijkstra[i],0,nodeCount*sizeof(unsigned char));
}

unsigned char Edge::getCoefficient(int src,int dest) {
	if ((src<0) || (src>nodeCount) || (dest<0) || (dest>nodeCount)) {
      printf("Error getCoefficient\n");
      return (0);
   }
   return coefficientsDijkstra[src][dest];
}

Links::Links(aid_t areaid,int n) {
	cnt=n;
	a_id=areaid;
	edges=(Edge ***) malloc(n*sizeof(Edge**));
	edgeExists=(unsigned char**)malloc(n*sizeof(unsigned char*)); 
        edgeHasCoefficients=(unsigned char**)malloc(n*sizeof(unsigned char*)); 
	for(int i=0;i<n;i++)
	{
		edges[i]=(Edge **) malloc(n*sizeof(Edge*));
		edgeExists[i]=(unsigned char*) malloc(n*sizeof(unsigned char)); 
                edgeHasCoefficients[i]=(unsigned char*) malloc(n*sizeof(unsigned char)); 
		memset(edgeExists[i],0,n*sizeof(unsigned char)); 
                memset(edgeHasCoefficients[i],0,n*sizeof(unsigned char)); 
	}
	edgeCount=0; 
	nextlnk=NULL;
}

Links::~Links() {
	for(int i=0;i<cnt;i++)
		for(int j=0;j<cnt;j++)
			if(edgeExists[i][j]) 
            delete edges[i][j];

	for(int i=0;i<cnt;i++) {
		free(edges[i]);
		free(edgeExists[i]);
      free(edgeHasCoefficients[i]);
	}

	free(edges);
	free(edgeExists);
   free(edgeHasCoefficients);

}

int Links::set(int trafficsrc,int trafficdest,int edgeSrc,int edgeDst) {
	if (edgeExists[edgeSrc][edgeDst]==0) { 
		edges[edgeSrc][edgeDst] = new Edge(cnt);
		edgeExists[edgeSrc][edgeDst]=1;
		edgeCount++;
	}

	if ((trafficsrc<0) || (trafficsrc>cnt) || (trafficdest<0) || (trafficdest>cnt))  {	
		printf("Links::set bad param trafficsrc %i trafficdest %i cnt %i\n", trafficsrc, trafficdest, cnt);
		return -1;
	}
	edgeHasCoefficients[edgeSrc][edgeDst]=1;
	return edges[edgeSrc][edgeDst]->setNewCoefficient(trafficsrc,trafficdest);
}

void Links:: reset()
{
   for(int i=0;i<cnt;i++)
      for(int j=0;j<cnt;j++) {
         edgeHasCoefficients[i][j]=0;
         if(edgeExists[i][j])   
         edges[i][j]->resetCoefficients() ;
      }

   edgeCount=0;
}


int Links:: getNodeCount() {
   return cnt;
}

int Links:: getEdgeCount() {
   edgeCount=0;
   for(int i=0;i<cnt;i++)
      for(int j=0;j<cnt;j++)
         if((edgeHasCoefficients[i][j])) 
            edgeCount++; 
   return edgeCount;
}


Edge* Links::getEdge(int edgeSrc,int edgeDst) {
   if(edgeExists[edgeSrc][edgeDst]) 
      return edges[edgeSrc][edgeDst];
   return NULL;
}




