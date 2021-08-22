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

#include <map>
#include <list>
#include <algorithm>

using namespace std;


double string_to_float( const std::string& s )
 {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     return 0;
   return x;
 } 

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


int getBinCount(char *file) {

   std::list<long> ovhFlows;
	
	getovhflows(file, ovhFlows);

   pugi::xml_document doc;
   pugi::xml_parse_result result = doc.load_file(file);

	pugi::xpath_node_set histogram = doc.select_nodes("/FlowMonitor/FlowStats/Flow");

	int bcount = 0;
	for (pugi::xpath_node_set::const_iterator it = histogram.begin(); it != histogram.end(); ++it)
	{
	   pugi::xpath_node node = *it;
	   string strflowId(node.node().attribute("flowId").value());
		long flowId = string_to_long( strflowId );
	   bool found = (std::find(ovhFlows.begin(), ovhFlows.end(), flowId) != ovhFlows.end());
		
		if (found)
		   continue;
	   for (pugi::xml_node histogram = node.node().child("delayHistogram"); histogram; histogram = histogram.next_sibling("delayHistogram")) {
	      const string bincount(histogram.attribute("nBins").value());
	      int bcount1 = string_to_int( bincount );
	      if (bcount < bcount1)
	         bcount = bcount1;
	  }
	}
	
	return bcount;

}

void fillBins(char *file, long *binarray, int bcount) {

   std::list<long> ovhFlows;
	
	getovhflows(file, ovhFlows);

   pugi::xml_document doc;
   pugi::xml_parse_result result = doc.load_file(file);

   pugi::xpath_node_set bins = doc.select_nodes("/FlowMonitor/FlowStats/Flow");

	
	memset(binarray, 0, bcount * sizeof(long));

	for (pugi::xpath_node_set::const_iterator it = bins.begin(); it != bins.end(); ++it)
	{
	
	   pugi::xpath_node node = *it;
	   string strflowId(node.node().attribute("flowId").value());
		long flowId = string_to_long( strflowId );
	   bool found = (std::find(ovhFlows.begin(), ovhFlows.end(), flowId) != ovhFlows.end());
		
		if (found)
		   continue;
	
	   for (pugi::xml_node histogram = node.node().child("delayHistogram"); histogram; histogram = histogram.next_sibling("delayHistogram")) {
	
	      for (pugi::xml_node histogrambin = histogram.child("bin"); histogrambin; histogrambin = histogrambin.next_sibling("bin")) {

		      const string bin(histogrambin.attribute("index").value());
		      const string packetcount(histogrambin.attribute("count").value());

		      int binid = string_to_int( bin );	
		         
		      long pcount = string_to_long( packetcount );	

		      binarray[binid] += pcount;
		   }
		
		}

	}
}

long totalBinCount(char *filebase) {

   int bcount = 0;
   for (int i = 1; i <=5; i++) {
      
      char *file = new char [strlen(filebase) + 10];
      sprintf(file, "%s%i.xml", filebase, i);
      
      int bcount1 = getBinCount(file);
	   
	   if (bcount < bcount1)
	      bcount = bcount1;
	   
	   delete file;
   }
   return bcount;
}

void getbinarray(char *filebase, int bcount, long *binarray) {

   for (int j = 0; j < bcount; j++)
      binarray[j] = 0;
   
   for (int i = 1; i <=5; i++) {
   
      char *file = new char [strlen(filebase) + 10];
      sprintf(file, "%s%i.xml", filebase, i);
   
      long *binarray1 = new long [bcount];
	fillBins(file, binarray1, bcount);
	
	   for (int j = 0; j < bcount; j++) {
	      binarray[j] += binarray1[j];
	   }
	
	 delete file;

   }
   
   for (int j = 0; j < bcount; j++) {
      binarray[j] /= 5;
   }

}

long *compressarray(long *binarray, int bcount, int compressionfactor) {

   long *carray = (long*)malloc(sizeof(long) * bcount / compressionfactor + 1);
   
   for (int i = 0; i < bcount / compressionfactor; i++) {
      carray[i] = 0;
   }
   
   for (int i = 0; i < bcount; i++) {
      carray[i/compressionfactor] += binarray[i];
   }
   
   return carray;
}


int main(int argc,char *argv[])
{
   
   int topoparam = atoi(argv[2]);
   
   int bcount = 0;
   
   char resultsfile[100];
   sprintf(resultsfile, "%s_ecmp", argv[1]);
   int bincount1 = totalBinCount(resultsfile);
   if (bincount1 > bcount)
      bcount = bincount1;
   
   sprintf(resultsfile, "%s_timp", argv[1]);
   bincount1 = totalBinCount(resultsfile);
   if (bincount1 > bcount)
      bcount = bincount1;
      
   sprintf(resultsfile, "%s_contra", argv[1]);
   bincount1 = totalBinCount(resultsfile);
   if (bincount1 > bcount)
      bcount = bincount1;
      
   if (topoparam > 3) {
      
   sprintf(resultsfile, "%s_dashpkt", argv[1]);
   bincount1 = totalBinCount(resultsfile);
   if (bincount1 > bcount)
      bcount = bincount1;
      
   sprintf(resultsfile, "%s_dashflowlet", argv[1]);
   bincount1 = totalBinCount(resultsfile);
   if (bincount1 > bcount)
      bcount = bincount1;
   
   }
   
   long *binarray = (long*)malloc(sizeof(long) * 5 * bcount);
   
   sprintf(resultsfile, "%s_ecmp", argv[1]);
   getbinarray(resultsfile, bcount, &(binarray[0]));

   sprintf(resultsfile, "%s_timp", argv[1]);
   getbinarray(resultsfile, bcount, &(binarray[bcount]));
   
   sprintf(resultsfile, "%s_contra", argv[1]);
   getbinarray(resultsfile, bcount, &(binarray[2*bcount]));
   
   if (topoparam > 3) {
   
   sprintf(resultsfile, "%s_dashpkt", argv[1]);
   getbinarray(resultsfile, bcount, &(binarray[3*bcount]));
   
   sprintf(resultsfile, "%s_dashflowlet", argv[1]);
   getbinarray(resultsfile, bcount, &(binarray[4*bcount]));

   }
   
   int compressionfactor = 10;
   long *ecmparray = compressarray(&(binarray[0]), bcount, compressionfactor);
   long *timparray = compressarray(&(binarray[bcount]), bcount, compressionfactor);
   long *contraarray = compressarray(&(binarray[2*bcount]), bcount, compressionfactor);
   long *dashpktarray = NULL;
   long *dashflowletarray = NULL; 
   if (topoparam > 3) {
   dashpktarray = compressarray(&(binarray[3*bcount]), bcount, compressionfactor);
   dashflowletarray = compressarray(&(binarray[4*bcount]), bcount, compressionfactor);
   }
   ostringstream ecmpqueues;
   ostringstream timpqueues;
   ostringstream contraqueues;
   ostringstream dashpktqueues;
   ostringstream dashflowletqueues;
   ostringstream queuesizes;
   for (int i = 0; i < bcount/compressionfactor; i++) {
      ecmpqueues << " " <<  ecmparray[i];
      timpqueues << " " <<  timparray[i]; 
      contraqueues << " " <<  contraarray[i];
      if (topoparam > 3) {
      dashpktqueues << " " <<  dashpktarray[i]; 
      dashflowletqueues << " " <<  dashflowletarray[i]; 
      }
      queuesizes << " " << i*10*compressionfactor;
   }

	ostringstream delays;
   ostringstream ticks;

   for (int i = 1; i <= bcount; i = i + 1) {
      delays << ", " <<  1 * i; 
      ticks << ", '" <<  10.0 * compressionfactor * i / 1000 << "'";

   }
   
   ofstream octavefile;
  	octavefile.open ("packetdelay.m");

	octavefile << "function packetdelay()" << endl;

	octavefile << "packetcount=[";
	for (int i = 0; i < bcount/compressionfactor; i++) {
		
		octavefile << ecmparray[i] << ",";
		octavefile << timparray[i] << ",";
		if (topoparam > 3) {
		octavefile << contraarray[i] << ",";
		octavefile << dashpktarray[i] << ",";
		octavefile << dashflowletarray[i];
		} else
		octavefile << contraarray[i] << ",";
      if (i < bcount-1)
         octavefile << ";";
   }

	octavefile << "]" << endl;
	
	octavefile << "sum(packetcount)" << endl;


	octavefile << "h=bar(packetcount);" << endl;
	octavefile << "set (h(1), 'facecolor', 'r');" << endl;
   octavefile << "set (h(2), 'facecolor', 'g');" << endl;
   octavefile << "set (h(3), 'facecolor', 'b');" << endl;
   if (topoparam > 3) {
   octavefile << "set (h(4), 'facecolor', 'magenta');" << endl;
   octavefile << "set (h(5), 'facecolor', 'yellow');" << endl;
	}
	octavefile << "set(gca, 'xtick', [" << delays.str() << "]);" << endl; 
	octavefile << "set(gca, 'xticklabel', {" << ticks.str() << "});" << endl; 
	octavefile << "xlabel('Packet Delay [ms]');" << endl;
	octavefile << "ylabel('Average number of packets');" << endl; 
	if (topoparam > 3) {
	octavefile << "legend('ECMP', 'TIMP', 'CONTRA', 'Dash packet', 'Dash flowlet', 'Location', 'northeast');" << endl;
	} else {
	octavefile << "legend('ECMP', 'TIMP', 'CONTRA', 'Location', 'northeast');" << endl;
	}
	
	octavefile << "print -djpg delayhistogram.jpg" << endl;
	octavefile << "endfunction" << endl;
	octavefile.close();
   
}
