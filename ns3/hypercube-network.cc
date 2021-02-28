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



#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/timp.h"
#include "ns3/timp-global.h"
#include "ns3/timp-helper.h"
#include "ns3/timp-helper.h"
#include "ns3/dfs.h"
#include "ns3/flow-monitor-helper.h"
#include <map>
#include <string>

#define FLOW_DATA_RATE "100Mbps"
#define FLOW_BYTES 30000
#define DATA_RATE_SWITCH "1000Mbps"

// eliminate server links as choking point
// because simulation uses only one server per switch
#define DATA_RATE "10000Mbps"

#define FLOWS_START 3.0

using namespace ns3;

Ptr<TimpGlobal> controller;

NodeContainer switches;
NodeContainer network;
NetDeviceContainer devices;
NodeContainer devicenodes;
NetDeviceContainer serverdevices;
NetDeviceContainer switchdevices;
Links *links;
Nodes *nodes;

NodeContainer servers;
Ipv4InterfaceContainer serverinterfaces;

char networkadr[100];

char *getnextnetwork();

std::map<std::string, int> flows;

std::map<std::string, int> packets;

std::ofstream durationfile;
std::ofstream flowplacefile;

int flowplace = 1;

bool flowecmp = false;

NS_LOG_COMPONENT_DEFINE ("TimpRouting");


void addtcpserver(int server1, NodeContainer &servers, int socket) {

   Address LocalAddress (InetSocketAddress (Ipv4Address::GetAny (), socket));
	PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", LocalAddress); 
	ApplicationContainer recvapp = packetSinkHelper.Install (servers.Get (server1));
	recvapp.Start (Seconds (2.0));
	recvapp.Stop (Seconds (10.0));

}

void addtcpclient(int server1, int server2, NodeContainer &servers, int socket) {

   BulkSendHelper	bulkSendHelper("ns3::TcpSocketFactory", Address ());
   bulkSendHelper.SetAttribute ("MaxBytes",  UintegerValue (FLOW_BYTES)); 
   ApplicationContainer appcont;
	Ptr<Ipv4> ipv4 = servers.Get(server1)->GetObject<Ipv4>();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
	Ipv4Address addri = iaddr.GetLocal ();
	AddressValue remoteAddress (InetSocketAddress (addri, socket));
	bulkSendHelper.SetAttribute ("Remote", remoteAddress);
	appcont.Add (bulkSendHelper.Install (servers.Get (server2)));
	appcont.Start (Seconds (FLOWS_START));
	appcont.Stop (Seconds (10.0));


}


void connect(NodeContainer *pplink, unsigned int *address1, unsigned int *address2, unsigned int *interface1, unsigned int *interface2) {

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE_SWITCH));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2us"));
	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (*pplink);

	Ipv4AddressHelper address;
	char *adr = getnextnetwork();
	address.SetBase (adr, "255.255.255.0");
	Ipv4InterfaceContainer p2pInterfaces;
	p2pInterfaces = address.Assign (p2pDevices);

	devices.Add(p2pDevices);
	devicenodes.Add(*pplink);
	switchdevices.Add(p2pDevices);

	Ipv4Address addr1 = p2pInterfaces.GetAddress(0, 0);
	Ipv4Address addr2 = p2pInterfaces.GetAddress(1, 0);

	*address1 = addr1.Get();
	*address2 = addr2.Get();
	
	Ptr<Ipv4> ipv4 = pplink->Get(0)->GetObject<Ipv4>();
	*interface1 = ipv4->GetInterfaceForAddress(addr1);
	ipv4 = pplink->Get(1)->GetObject<Ipv4>();
	*interface2 = ipv4->GetInterfaceForAddress(addr2);
	
}


void connectsw(unsigned int i, unsigned int curlevel) {


   std::cout << "connectsw " << i << " " << curlevel << std::endl;

   NodeContainer newlink;

	newlink.Add(switches.Get(i));
	newlink.Add(switches.Get(curlevel));	
			
	unsigned int address1;
	unsigned int address2;
	unsigned int interface1;
	unsigned int interface2;
	connect(&newlink, &address1, &address2, &interface1, &interface2);

   nodes->addlink((switches.Get(i))->GetId(), (switches.Get(curlevel))->GetId());		        
   nodes->setlinkends((switches.Get(i))->GetId(), (switches.Get(curlevel))->GetId(), address1, address2, interface1, interface2);


}


void hypercube() {

   switches.Create(16);
   network.Add(switches);

   TimpHelper timpRouting;
   Ipv4ListRoutingHelper listRH;
   listRH.Add (timpRouting, 0);

   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp)
   internet.SetRoutingHelper (listRH);
   internet.Install (switches);
     
     
   connectsw(0, 1);
   connectsw(0, 2);
   connectsw(0, 4);
   connectsw(0, 8);
   connectsw(1, 3);
   connectsw(1, 5);
   connectsw(1, 9);
   connectsw(2, 3);
   connectsw(2, 6);
   connectsw(2, 10);
   connectsw(3, 7);
   connectsw(3, 11);
   connectsw(4, 5);
   connectsw(4, 6);
   connectsw(4, 12);
   connectsw(5, 7);
   connectsw(5, 13);
   connectsw(6, 7);  
   connectsw(6, 14);  
   connectsw(7, 15);
   connectsw(8, 9);
   connectsw(8, 10);
   connectsw(8, 12);
   connectsw(9, 11);
   connectsw(9, 13);
   connectsw(10, 11);
   connectsw(10, 14);
   connectsw(11, 15);
   connectsw(12, 13);        
   connectsw(12, 14);
   connectsw(13, 15);
   connectsw(14, 15);                        
     
}

char *getnextnetwork() {

	static int i = 1;
	static int j = 0;
	
	j++;
	if (j==255) {
		if (i==255) 
			printf("addresses exausted\n");
		i++;
		j=1;	
	}

	sprintf(networkadr, "10.%i.%i.0", i, j);
	
	return networkadr;
}

void addservers(int swservers) {

	int sw = switches.GetN();

  InternetStackHelper internetNodes;
  internetNodes.SetIpv6StackInstall (false);
  internetNodes.Install (servers);

	for (int i = 0; i < sw; i++) {

		for (int j=0; j<swservers; j++) {

			NodeContainer p2pNodes;

			p2pNodes.Add (switches.Get(i));
			p2pNodes.Create (1);			

			internetNodes.Install(p2pNodes.Get(1));
			
			PointToPointHelper pointToPoint;
			pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE));
			pointToPoint.SetChannelAttribute ("Delay", StringValue ("2us"));
			NetDeviceContainer p2pDevices;

			p2pDevices = pointToPoint.Install (p2pNodes);

			Ipv4AddressHelper address;
			char *adr = getnextnetwork();
			address.SetBase (adr, "255.255.255.0");
			Ipv4InterfaceContainer p2pInterfaces;
			p2pInterfaces = address.Assign (p2pDevices);

			servers.Add(p2pNodes.Get(1));
			serverinterfaces.Add(p2pInterfaces.Get(1));
			network.Add(p2pNodes.Get(1));
	
			devices.Add(p2pDevices);
			devicenodes.Add(p2pNodes);
			serverdevices.Add(p2pDevices.Get(1));

			Ipv4Address addr1 = p2pInterfaces.GetAddress(0, 0);
			Ipv4Address addr2 = p2pInterfaces.GetAddress(1, 0);
			unsigned int address1 = addr1.Get();
			unsigned int address2 = addr2.Get();
			Ptr<Ipv4> ipv4 = p2pNodes.Get(0)->GetObject<Ipv4>();
			unsigned int interface1 = ipv4->GetInterfaceForAddress(addr1);
			ipv4 = p2pNodes.Get(1)->GetObject<Ipv4>();
			unsigned int interface2 = ipv4->GetInterfaceForAddress(addr2);
			std::cout << "add server " << (p2pNodes.Get(0))->GetId() << " " << (p2pNodes.Get(1))->GetId() << std::endl;
         nodes->addlink((p2pNodes.Get(0))->GetId(), (p2pNodes.Get(1))->GetId());		
			nodes->setlinkends((p2pNodes.Get(0))->GetId(), (p2pNodes.Get(1))->GetId(), address1, address2, interface1, interface2);
		}

	}

}

void RxTrace(Ptr<const Packet> packet, const Address &address, const Address &localAddress) {
   InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (address);
   InetSocketAddress localIaddr = InetSocketAddress::ConvertFrom (localAddress);

   std::stringstream ss;
   ss << " RxTrace " << iaddr.GetIpv4() << ":" << iaddr.GetPort() << " local " << localIaddr.GetIpv4() << ":" << localIaddr.GetPort() << " ";
   std::string s = ss.str();
   if ( flows.find(s) == flows.end() ) 
      flows[s] = packet->GetSize();
   else
      flows[s] += packet->GetSize();
   packets[s]++;
   if (flows[s] >= FLOW_BYTES) {
      Time duration = Simulator::Now () - Seconds (FLOWS_START);    
      std::cout << " flow finished, duration " << duration.GetMicroSeconds() << s << " " << Simulator::Now ().GetMicroSeconds() << " flows[s] " << flows[s] << " packets[s] " << packets[s] << std::endl;

      durationfile << " " <<  duration.GetMicroSeconds();
      flowplacefile << " " <<  flowplace++;

   }
}

int main (int argc, char **argv)
{
  
   int oldicount = 0;

   CommandLine cmd;
   cmd.AddValue ("flowecmp", "Flow based ECMP or TIMP", flowecmp);
   cmd.AddValue ("oldicount", "Number of OLDI-s executing simultaneously", oldicount);
   cmd.Parse (argc, argv);

   remove( "routes1.txt" );

   remove( "flowduration.txt" );
   remove( "flowcount.txt" );

   int swcount = 16;
   int devcount = 2*swcount; // two servers per switch
   std::cout << "devcount " << devcount << std::endl;
   links = new Links(0, devcount);
   nodes = new Nodes(devcount);

   //circle(swcount);
   hypercube();

	// add servers
	addservers(1); 

   nodes->topologyfinished();
   nodes->nDepthFirstSearch(links);

   std::list< int > torlist;
   for (int i = 0; i < devcount; i++)
      torlist.push_back(i);  

  
   TimpHelper timpRouting;

   if (!flowecmp) {
      controller = timpRouting.GetTimpGlobal ();
      controller->Initialize(network, torlist);
   } else {
      Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting",BooleanValue(true));
	   Ipv4GlobalRoutingHelper::PopulateRoutingTables();
   }

   // default routes for servers
   if (!flowecmp) {
      Ptr<Ipv4StaticRouting> staticRouting;
      char adr[100];
      for (int i = 0; i < swcount; i++) {
         staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (servers.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
         sprintf(adr, "10.1.%i.1", i+29);   
         staticRouting->SetDefaultRoute (Ipv4Address (adr), 1 ); 
      }
   }

   NS_LOG_INFO ("Create Applications.");

   for (int x = 0; x < oldicount; x++) {
      addtcpserver(0, servers, 9000+x);
      addtcpclient(0, 3, servers, 9000+x);
      addtcpclient(0, 4, servers, 9000+x);
      addtcpclient(0, 5, servers, 9000+x);
      addtcpclient(0, 6, servers, 9000+x);
      addtcpclient(0, 7, servers, 9000+x);
      addtcpclient(0, 8, servers, 9000+x);
      addtcpclient(0, 9, servers, 9000+x);
      addtcpclient(0, 10, servers, 9000+x);
      addtcpclient(0, 11, servers, 9000+x);
   }

   durationfile.open ("flowduration.txt", std::ios::out | std::ios::app);
   flowplacefile.open ("flowcount.txt", std::ios::out | std::ios::app);

   AsciiTraceHelper ascii;

   PointToPointHelper csma;

   csma.EnableAsciiAll (ascii.CreateFileStream ("timp-simple-routing.tr"));
   csma.EnablePcapAll ("timp-simple-routing", true);
   
   Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback(&RxTrace));


   NS_LOG_INFO ("Run Simulation.");
   Simulator::Stop (Seconds (131.0));
   Simulator::Run ();

   if (!flowecmp) 
      controller->PrintStatistics();

   Simulator::Destroy ();

   durationfile.close();
   flowplacefile.close();

   NS_LOG_INFO ("Done.");
}

