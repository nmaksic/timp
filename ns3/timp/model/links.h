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

#ifndef LINKS
#define LINKS

typedef unsigned int uns32;
typedef unsigned short uns16;
typedef unsigned char byte;

typedef uns32 aid_t;
typedef uns32 rtid_t;

// Keeps Dijkstra coefficients for one graph edge and for all combinations of switches which communicate over that edge.
class Edge
{
   int nodeCount; 
   unsigned char **coefficientsDijkstra;
public:
   Edge(int nodeCount);
   ~Edge();
   int setNewCoefficient(int src,int dest); 
   void resetCoefficients();//vraca sve koeficijente na nulu u matrici za linkove
   unsigned char getCoefficient(int src,int dest);
};


// Keeps all Dijkstra coefficients in the networ
class Links
{
   Edge ***edges;
   unsigned char **edgeExists;
   unsigned char **edgeHasCoefficients;
   int edgeCount;
public:
   int cnt;
   aid_t a_id;
   Links *nextlnk;
   Links(aid_t areaid,int n);
   ~Links();
   int set(int trafficsrc,int trafficdest,int edgeSrc,int edgeDst); 
   Edge* getEdge(int edgeSrc,int edgeDst);
   int getNodeCount(); 
   int getEdgeCount();
   void reset(); 
};





#endif
