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

#include "pugixml-1.11/src/pugixml.hpp"

#include <iostream>
#include <string.h>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <algorithm>

using namespace std;


int string_to_int( const std::string& s )
 {
   std::istringstream i(s);
   int x;
   if (!(i >> x))
     return 0;
   return x;
 } 

long string_to_long( const std::string& s )
 {
   std::istringstream i(s);
   long x;
   if (!(i >> x))
     return 0;
   return x;
 } 


void getovhflows(char *file, std::list<long> &ovhFlows) {
   pugi::xml_document doc;

   pugi::xml_parse_result result = doc.load_file(file);

	int flowcount = 0;

	pugi::xpath_node_set flows = doc.select_nodes("/FlowMonitor/Ipv4FlowClassifier/Flow");

	for (pugi::xpath_node_set::const_iterator it = flows.begin(); it != flows.end(); ++it)
	{
		pugi::xpath_node node = *it;

		string strprotocol(node.node().attribute("protocol").value());

		long protocol = string_to_long( strprotocol );

		string strdestinationport(node.node().attribute("destinationPort").value());

		long destinationport = string_to_long( strdestinationport );
		
		string strflowId(node.node().attribute("flowId").value());

		long flowId = string_to_long( strflowId );
		
		if (protocol == 17 && destinationport == 11001) {
		   ovhFlows.push_back (flowId);
		}
		
		flowcount++;
		

	}
}

void ovhpackets(char *file, long &total, long &ovh) {

   pugi::xml_document doc;

   pugi::xml_parse_result result = doc.load_file(file);

	int flowcount = 0;
	
	std::list<long> ovhFlows;
	
	getovhflows(file, ovhFlows);

   total = 0;
   ovh = 0;

	pugi::xpath_node_set flows = doc.select_nodes("/FlowMonitor/FlowStats/Flow");

	for (pugi::xpath_node_set::const_iterator it = flows.begin(); it != flows.end(); ++it)
	{
		pugi::xpath_node node = *it;

		string strtxpackets(node.node().attribute("txBytesEachPort").value());

		long txpackets = string_to_long( strtxpackets );

		total += txpackets;
		
		string strflowId(node.node().attribute("flowId").value());

		long flowId = string_to_long( strflowId );
		
		bool found = (std::find(ovhFlows.begin(), ovhFlows.end(), flowId) != ovhFlows.end());
		
		if (found)
		   ovh += txpackets;

		flowcount++;
		

	}
}

void getovhratio(char *filebase, long &totalovh, long &totalpackets) {

   totalpackets = 0;
   totalovh = 0;
   for (int i = 1; i <=5; i++) {
   
      char *file = new char [strlen(filebase) + 10];
      sprintf(file, "%s%i.xml", filebase, i);
   
      long total = 0;
	   long ovh = 0; 

      ovhpackets(file, total, ovh);
      
      totalpackets += total;
      totalovh += ovh;
	
	 delete file;

   }
   
   totalovh /= 5;
   totalpackets /= 5;
   
}



int main(int argc,char *argv[])
{
	
	int topoparam = atoi(argv[2]);
   
   std::cout << "topoparam " << topoparam << std::endl;
   
   int bcount = 0;
   
   long totaltxecmp = 0;
	long totalovhecmp = 0; 
   char resultsfile[100];
   sprintf(resultsfile, "%s_ecmp", argv[1]);
   getovhratio(resultsfile, totalovhecmp, totaltxecmp);
   std::cout << "overhead ecmp " <<  totalovhecmp << " totaltx " << totaltxecmp << " percent " << 100.0*totalovhecmp/totaltxecmp << std::endl;
   
   long totaltxtimp = 0;
	long totalovhtimp = 0; 
   sprintf(resultsfile, "%s_timp", argv[1]);
   getovhratio(resultsfile, totalovhtimp, totaltxtimp);
   std::cout << "overhead timp " <<  totalovhtimp << " totaltx " << totaltxtimp << " percent " << 100.0*totalovhtimp/totaltxtimp << std::endl;
   
   long totaltxcontra = 0;
	long totalovhcontra = 0; 
   sprintf(resultsfile, "%s_contra", argv[1]);
   getovhratio(resultsfile, totalovhcontra, totaltxcontra);
   std::cout << "overhead contra " <<  totalovhcontra << " totaltx " << totaltxcontra << " percent " << 100.0*totalovhcontra/totaltxcontra << std::endl;
   
   if (topoparam > 3) {
   
   long totaltxdashpkt = 0;
	long totalovhdashpkt = 0; 
   sprintf(resultsfile, "%s_dashpkt", argv[1]);
   getovhratio(resultsfile, totalovhdashpkt, totaltxdashpkt);
   std::cout << "overhead dashpkt " <<  totalovhdashpkt << " totaltx " << totaltxdashpkt << " percent " << 100.0*totalovhdashpkt/totaltxdashpkt << std::endl;
   
   long totaltxdashflowlet = 0;
	long totalovhdashflowlet = 0; 
   sprintf(resultsfile, "%s_dashflowlet", argv[1]);
   getovhratio(resultsfile, totalovhdashflowlet, totaltxdashflowlet);
   std::cout << "overhead dashflowlet " <<  totalovhdashflowlet << " totaltx " << totaltxdashflowlet << " percent " << 100.0*totalovhdashflowlet/totaltxdashflowlet << std::endl;
   
   }

}
