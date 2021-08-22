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

void droppedratio(char *file, long &totaldropped, long &totaltx) {

   pugi::xml_document doc;

   pugi::xml_parse_result result = doc.load_file(file);

	int flowcount = 0;

	pugi::xpath_node_set flows = doc.select_nodes("/FlowMonitor/FlowStats/Flow");

	for (pugi::xpath_node_set::const_iterator it = flows.begin(); it != flows.end(); ++it)
	{
		pugi::xpath_node node = *it;

		string strtxpackets(node.node().attribute("txPackets").value());

		long txpackets = string_to_long( strtxpackets );

		totaltx += txpackets;
		
		string strlostpackets(node.node().attribute("lostPackets").value());

		long lostpackets = string_to_long( strlostpackets );
		
		totaldropped += lostpackets;

		if (lostpackets > 0)
			cout << "lost packets" << endl; 

		flowcount++;
		

	}
}

void getdroppedratio(char *filebase, long &totaldropped, long &totaltx) {

   totaldropped = 0;
   totaltx = 0;
   for (int i = 1; i <=5; i++) {
   
      char *file = new char [strlen(filebase) + 10];
      sprintf(file, "%s%i.xml", filebase, i);
   
      long tx = 0;
	   long dropped = 0; 

      droppedratio(file, dropped, tx);
      
      totaldropped += dropped;
      totaltx += tx;
	
	 delete file;

   }
   
}



int main(int argc,char *argv[])
{

	int topoparam = atoi(argv[2]);
   
   int bcount = 0;
   
   long totaltxecmp = 0;
	long totaldroppedecmp = 0; 
   char resultsfile[100];
   sprintf(resultsfile, "%s_ecmp", argv[1]);
   getdroppedratio(resultsfile, totaldroppedecmp, totaltxecmp);
   std::cout << "dropped ecmp " <<  totaldroppedecmp << " totaltx " << totaltxecmp << std::endl;
   
   long totaltxtimp = 0;
	long totaldroppedtimp = 0; 
   sprintf(resultsfile, "%s_timp", argv[1]);
   getdroppedratio(resultsfile, totaldroppedtimp, totaltxtimp);
   std::cout << "dropped timp " <<  totaldroppedtimp << " totaltx " << totaltxecmp << std::endl;
   
   long totaltxcontra = 0;
	long totaldroppedcontra = 0; 
   sprintf(resultsfile, "%s_contra", argv[1]);
   getdroppedratio(resultsfile, totaldroppedcontra, totaltxcontra);
   std::cout << "dropped contra " <<  totaldroppedcontra << " totaltx " << totaltxecmp << std::endl;
   
   if (topoparam > 3) {
   
   long totaltxdashpkt = 0;
	long totaldroppeddashpkt = 0; 
   sprintf(resultsfile, "%s_dashpkt", argv[1]);
   getdroppedratio(resultsfile, totaldroppeddashpkt, totaltxdashpkt);
   std::cout << "dropped dashpkt " <<  totaldroppeddashpkt << " totaltx " << totaltxecmp << std::endl;
   
   long totaltxdashflowlet = 0;
	long totaldroppeddashflowlet = 0; 
   sprintf(resultsfile, "%s_dashflowlet", argv[1]);
   getdroppedratio(resultsfile, totaldroppeddashflowlet, totaltxdashflowlet);
   std::cout << "dropped dashflowlet " <<  totaldroppeddashflowlet << " totaltx " << totaltxecmp << std::endl;
   
   }

}
