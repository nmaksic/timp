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

double string_to_double( const std::string& s )
 {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     return 0;
   return x;
 } 

void longestflow(char *file, long &flowid, double *ovhFlows) {

   pugi::xml_document doc;

   pugi::xml_parse_result result = doc.load_file(file);

	int flowcount = 0;

   double longestduration = 0;
   
   flowid = 0;
   
   int i = 0;

	pugi::xpath_node_set flows = doc.select_nodes("/FlowMonitor/FlowStats/Flow");

	for (pugi::xpath_node_set::const_iterator it = flows.begin(); it != flows.end(); ++it)
	{
		pugi::xpath_node node = *it;

	   string strflowId(node.node().attribute("flowId").value());
	   
		long flowId1 = string_to_long( strflowId );
		

		string strtxpackets(node.node().attribute("timeFirstTxPacket").value());

      string firsttxstr = strtxpackets.substr (1,strtxpackets.length()-7);   // "+3.00818e+06ns"

		double firsttx = string_to_double( firsttxstr );

		string strlostpackets(node.node().attribute("timeLastTxPacket").value());

      string lastrxstr = strlostpackets.substr (1,strlostpackets.length()-7);   // "+3.00818e+06ns"

      double lastrx = string_to_double( lastrxstr );

      if (firsttx == 3) {
      if (longestduration < lastrx) {
      longestduration = lastrx;
      flowid = flowId1;
      }
      ovhFlows[i++] = lastrx-firsttx;
      }

		flowcount++;
		

	}
	
	pugi::xml_document doc1;
	for (pugi::xpath_node_set::const_iterator it = flows.begin(); it != flows.end(); ++it)
	{
	   pugi::xpath_node node = *it;

	   string strflowId(node.node().attribute("flowId").value());
	   
		long flowId1 = string_to_long( strflowId );
		
		if (flowId1 == flowid) {
		   pugi::xml_document doc1;
		   doc1.append_copy(node.node());
		   //doc1.print(cout);
		}
	}
}

void printflowid(char *filebase, double *avgflows) {

   
   for (int j = 0; j < 100; j++) 
      avgflows[j] = 0;

   for (int i = 1; i <=5; i++) {
   
      char *file = new char [strlen(filebase) + 10];
      sprintf(file, "%s%i.xml", filebase, i);
   
      long longestflowid = 0;

      double ovhFlows[100];

      longestflow(file, longestflowid, ovhFlows);
      
      printf("file %s longest flowid %li\n", file, longestflowid);

      for (int j = 0; j < 100; j++) {
         avgflows[j] += ovhFlows[j];
      }
      
	   delete file;

   }
   
   for (int j = 0; j < 100; j++) {
      avgflows[j] /= 5;
   }
   
}



int main(int argc,char *argv[])
{

	int topoparam = atoi(argv[2]);
   
   int bcount = 0;
   
   char resultsfile[100];
   
   double avgflows[100];
   
   long totaltxecmp = 0;
	long totaldroppedecmp = 0; 
   sprintf(resultsfile, "%s_ecmp", argv[1]);
   printflowid(resultsfile, avgflows);
   sort(avgflows, avgflows + 100);
   for (int i = 0; i < 100; i++)
      std::cout << avgflows[i] << " ";
   std::cout << std::endl;
   ofstream plotfile;
  	plotfile.open ("file1.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << avgflows[i] << endl;
   plotfile.close();
   
   double ecmparray[100];
   for (int i = 0; i < 100; i++)
      ecmparray[i] = avgflows[i];
   
   long totaltxtimp = 0;
	long totaldroppedtimp = 0; 
   sprintf(resultsfile, "%s_timp", argv[1]);
   printflowid(resultsfile, avgflows);
   sort(avgflows, avgflows + 100);
   for (int i = 0; i < 100; i++)
      std::cout << avgflows[i] << " ";
   std::cout << std::endl;
   plotfile.open ("file2.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << avgflows[i] << endl;
   plotfile.close();
      
   double timparray[100];
   for (int i = 0; i < 100; i++)
      timparray[i] = avgflows[i];
   
   long totaltxcontra = 0;
	long totaldroppedcontra = 0; 
   sprintf(resultsfile, "%s_contra", argv[1]);
   printflowid(resultsfile, avgflows);
   sort(avgflows, avgflows + 100);
   for (int i = 0; i < 100; i++)
      std::cout << avgflows[i] << " ";
   std::cout << std::endl;
   plotfile.open ("file3.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << avgflows[i] << endl;
   plotfile.close();
   
   double contraarray[100];
   double dashflowletarray[100];
   for (int i = 0; i < 100; i++)
      contraarray[i] = avgflows[i];
   
   double dashpktarray[100];
   if (topoparam > 3) {
   
   long totaltxdashpkt = 0;
	long totaldroppeddashpkt = 0; 
   sprintf(resultsfile, "%s_dashpkt", argv[1]);
   printflowid(resultsfile, avgflows);
   sort(avgflows, avgflows + 100);
   for (int i = 0; i < 100; i++)
      std::cout << avgflows[i] << " ";
   std::cout << std::endl;
   plotfile.open ("file4.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << avgflows[i] << endl;
   plotfile.close();
   
   for (int i = 0; i < 100; i++)
      dashpktarray[i] = avgflows[i];
   
   long totaltxdashflowlet = 0;
	long totaldroppeddashflowlet = 0; 
   sprintf(resultsfile, "%s_dashflowlet", argv[1]);
   printflowid(resultsfile, avgflows);
   sort(avgflows, avgflows + 100);
   for (int i = 0; i < 100; i++)
      std::cout << avgflows[i] << " ";
   std::cout << std::endl;
   plotfile.open ("file5.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << avgflows[i] << endl;
   plotfile.close();
   
   for (int i = 0; i < 100; i++)
      dashflowletarray[i] = avgflows[i];
   
   }
   
   plotfile.open ("file.dat");
  	for (int i = 0; i < 100; i++)
      plotfile << i+1 << " " << ecmparray[i] << " " << timparray[i] << " " << contraarray[i] << " " << dashpktarray[i] << " " << dashflowletarray[i] << endl;
   plotfile.close();
   
   ostringstream ecmpqueues;
   ostringstream timpqueues;
   ostringstream contraqueues;
   ostringstream dashpktqueues;
   ostringstream dashflowletqueues;
   ostringstream queuesizes;
   for (int i = 0; i < 100; i++) {
      ecmpqueues << " " <<  ecmparray[i];
      timpqueues << " " <<  timparray[i]; //[bcount+i];
      contraqueues << " " <<  contraarray[i]; //binarray[2*bcount+i];
      if (topoparam > 3) {
      dashpktqueues << " " <<  dashpktarray[i]; //binarray[3*bcount+i];
      dashflowletqueues << " " <<  dashflowletarray[i]; //binarray[4*bcount+i];
      }
      queuesizes << " " << i+1;
   }
   
   ofstream octavefile;
  	octavefile.open ("fct.m");

	octavefile << "function fct()" << endl;
	
	octavefile << "graphics_toolkit(\"gnuplot\")" << endl;

   octavefile << "ecmpqueues=[ " << ecmpqueues.str() << "];" << endl;
   octavefile << "timpqueues=[ " << timpqueues.str() << "];" << endl;
   octavefile << "contraqueues=[ " << contraqueues.str() << "];" << endl;
   if (topoparam > 3) {
   octavefile << "dashpktqueues=[ " << dashpktqueues.str() << "];" << endl;
   octavefile << "dashflowletqueues=[ " << dashflowletqueues.str() << "];" << endl;
   }
   octavefile << "queuesizes=[ " << queuesizes.str() << "];" << endl;


   if (topoparam > 3) {
	octavefile << "plot(queuesizes, ecmpqueues, '-', queuesizes, timpqueues, '--', queuesizes, contraqueues, '-.', queuesizes, dashpktqueues, '-.', queuesizes, dashflowletqueues, '-.');" << endl;
	octavefile << "legend('ECMP', 'TIMP', 'CONTRA', 'Dash packet', 'Dash flowlet', 'Location', 'northwest');" << endl;
	} else {
	octavefile << "plot(queuesizes, ecmpqueues, '-', queuesizes, timpqueues, '--', queuesizes, contraqueues, '-.');" << endl;
	octavefile << "legend('ECMP', 'TIMP', 'CONTRA', 'Location', 'northwest');" << endl;
	}
	octavefile << "xlabel('Flow arrival order');" << endl;
	octavefile << "ylabel('fct');" << endl;
	octavefile << "print -djpg fct.jpg" << endl;
	octavefile << "endfunction" << endl;
	octavefile.close();

}
