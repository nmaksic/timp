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
#include "ns3/dfs.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/contra.h"
#include "ns3/contra-global.h"
#include "ns3/contra-helper.h"
#include "ns3/dash.h"
#include "ns3/dash-global.h"
#include "ns3/dash-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/traffic-control-module.h"
#include <map>
#include <string>

#define FLOW_BYTES 30000
#define DATA_RATE_SWITCH "10000Mbps"

#define DATA_RATE "10000Mbps"

//#define FLOWS_START_BACKGROUND 0.002
//#define FLOWS_START 0.008
#define FLOWS_START_BACKGROUND 0.001
#define FLOWS_START 0.003

#define SIM_DURATION 0.006
#define SIM_DURATION_ECMP 0.011
#define SERVER_START 0.0

#define SWSERVERS 42

#define PROBE_PERIOD 0.000256

using namespace ns3;

Ptr<TimpGlobal> controller;
Ptr<ContraGlobal> contraGlobal;
Ptr<DashGlobal> dashGlobal;

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

std::map<std::string, std::pair<int, double>> flowsprops; // flowname, flowsize, flow start 

std::ofstream durationfile;
std::ofstream flowplacefile;

int flowplace = 1;

bool flowecmp = false;

#define ROUTING_TIMP 1
#define ROUTING_CONTRA 2
#define ROUTING_DASH_PACKET 3
#define ROUTING_DASH_FLOWLET 4


int routing = 0; // 1 - timp, 2 - contra, 3 - dash packet, 4 - dash flowlet

std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >> switchNeighbours;
std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> switchNetworks;



NS_LOG_COMPONENT_DEFINE ("TimpRouting");


void AddNeighbor (int dstid, uint32_t interface, Ipv4Address adr, int nbrid) {

   std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >>::iterator it;
   for (it = switchNeighbours.begin (); it != switchNeighbours.end (); it++)
    {
      if (it->first == dstid) {
         break;   
      }
    }
   if (it == switchNeighbours.end ()) {
      switchNeighbours.push_back(std::make_pair (dstid, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>())); 
      it = switchNeighbours.end ();
      it--;
   }
   

   unsigned int ifc = (unsigned int)interface;
   auto it1 =  std::find_if(it->second.begin(), it->second.end(), [&ifc](const std::tuple<uint32_t, Ipv4Address, uint32_t>& element){ return std::get<0>(element) == ifc;});
   bool found = (it1 != it->second.end());
   if (!found)
      it->second.push_back(std::make_tuple (interface, adr, nbrid));

}

void AddNetwork (int dstid, Ipv4Address adr, Ipv4Mask mask) {

   dstid = dstid;

   std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >>::iterator it;
   for (it = switchNetworks.begin (); it != switchNetworks.end (); it++)
    {
      if (it->first == dstid) {
         break;   
      }
    }
   if (it == switchNetworks.end ()) {
      switchNetworks.push_back(std::make_pair (dstid, std::list<std::pair <Ipv4Address, Ipv4Mask>>())); 
      it = switchNetworks.end ();
      it--;
   }
   

   auto it1 =  std::find_if(it->second.begin(), it->second.end(), [&adr, &mask](const std::pair<Ipv4Address, Ipv4Mask>& element){ return element.first == adr && element.second == mask;});
   bool found = (it1 != it->second.end());
   if (!found)
      it->second.push_back(std::make_pair (adr, mask));

}


void addtcpserver(int server1, NodeContainer &servers, int socket) {

   Address LocalAddress (InetSocketAddress (Ipv4Address::GetAny (), socket));
	PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", LocalAddress); 
	ApplicationContainer recvapp = packetSinkHelper.Install (servers.Get (server1));
	recvapp.Start (Seconds (SERVER_START));
	recvapp.Stop (Seconds (10.0));

}

std::string getflowid(InetSocketAddress serverhost) {

   std::stringstream ss;
   ss << " flow " << serverhost.GetIpv4() << ":" << serverhost.GetPort();
   return ss.str();

}

void addtcpclient(int server1, int server2, NodeContainer &servers, int socket, double flowstart, int flowbytes) {

   BulkSendHelper	bulkSendHelper("ns3::TcpSocketFactory", Address ());
   bulkSendHelper.SetAttribute ("MaxBytes",  UintegerValue (flowbytes)); 
   ApplicationContainer appcont;
	Ptr<Ipv4> ipv4 = servers.Get(server1)->GetObject<Ipv4>();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
	Ipv4Address addri = iaddr.GetLocal ();
	AddressValue remoteAddress (InetSocketAddress (addri, socket));
	bulkSendHelper.SetAttribute ("Remote", remoteAddress);
	appcont.Add (bulkSendHelper.Install (servers.Get (server2)));
	appcont.Start (Seconds (flowstart));
	appcont.Stop (Seconds (10.0));
	
	
	std::string flowkey = getflowid(InetSocketAddress (addri, socket));
   std::pair<int,double> f = std::make_pair(flowbytes, flowstart);
   flowsprops.insert ( std::pair<std::string,std::pair<int,double>>(flowkey,f) );
	
}


void connect(NodeContainer *pplink, unsigned int *address1, unsigned int *address2, unsigned int *interface1, unsigned int *interface2) {

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE_SWITCH));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2us"));
	pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
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
	
	AddNeighbor (pplink->Get(0)->GetId(), *interface1, addr2, pplink->Get(1)->GetId());
	AddNeighbor (pplink->Get(1)->GetId(), *interface2, addr1, pplink->Get(0)->GetId());
	
	AddNetwork (pplink->Get(0)->GetId(), addr1, "255.255.255.0");
	AddNetwork (pplink->Get(1)->GetId(), addr2, "255.255.255.0");
}


void connectsw(unsigned int i, unsigned int curlevel) {

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
   ContraHelper contraRouting;
   DashHelper dashRouting;
   Ipv4ListRoutingHelper listRH;
   if (routing == ROUTING_TIMP)
      listRH.Add (timpRouting, 0);
   if (routing == ROUTING_CONTRA)
      listRH.Add (contraRouting, 0);
   if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
      listRH.Add (dashRouting, 0);


   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp){
      if (routing == ROUTING_TIMP)
         internet.SetRoutingHelper (listRH);
      if (routing == ROUTING_CONTRA)
         internet.SetRoutingHelper (listRH); 
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
         internet.SetRoutingHelper (listRH);   
   }
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

void butterfly(int fbd) {

	switches.Create(pow(2, fbd));

	network.Add(switches);

   TimpHelper timpRouting;
   ContraHelper contraRouting;
   DashHelper dashRouting;
   Ipv4ListRoutingHelper listRH;
   if (routing == ROUTING_TIMP)
      listRH.Add (timpRouting, 0);
   if (routing == ROUTING_CONTRA)
      listRH.Add (contraRouting, 0);
   if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
      listRH.Add (dashRouting, 0);


   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp){
      if (routing == ROUTING_TIMP)
         internet.SetRoutingHelper (listRH);
      if (routing == ROUTING_CONTRA)
         internet.SetRoutingHelper (listRH); 
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
         internet.SetRoutingHelper (listRH);   
   }
   internet.Install (switches);
     

	for (unsigned long i = 0; i < pow(2, fbd); i++) {
		
		for (int j = 0; j < fbd; j++) {
			unsigned long b = pow(2, j);
			unsigned long curlevel = i & b ? i & !b : i | b;

			if (curlevel > i) {

				std::cout << "connect " << i << " and " << curlevel  << std::endl;	

            connectsw(i, curlevel);

			}
		}
	}
		
}

void torus(unsigned int swdimension) {

   switches.Create(swdimension*swdimension);
   network.Add(switches);

   TimpHelper timpRouting;
   ContraHelper contraRouting;
   DashHelper dashRouting;
   Ipv4ListRoutingHelper listRH;
   if (routing == ROUTING_TIMP)
      listRH.Add (timpRouting, 0);
   if (routing == ROUTING_CONTRA)
      listRH.Add (contraRouting, 0);
   if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
      listRH.Add (dashRouting, 0);


   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp){
      if (routing == ROUTING_TIMP)
         internet.SetRoutingHelper (listRH);
      if (routing == ROUTING_CONTRA)
         internet.SetRoutingHelper (listRH); 
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
         internet.SetRoutingHelper (listRH);   
   }
   internet.Install (switches);
     
     
   for (unsigned long i = 0; i < swdimension - 1; i++) {
     for (unsigned long j = 0; j < swdimension - 1; j++) {
      
      unsigned long cursv = i+j*swdimension;
      
      connectsw(cursv, cursv+1);
      connectsw(cursv, cursv+swdimension);

      
      }
   }
   
   for (unsigned long i = 1; i < swdimension; i++) {
   
      connectsw(i*swdimension - 1, (i+1)*swdimension - 1);
      connectsw(swdimension*(swdimension - 1)+i-1, swdimension*(swdimension - 1)+i);
   }
   
   for (unsigned long i = 1; i <= swdimension; i++) {
   
      connectsw((i-1)*swdimension, i*swdimension - 1);
      connectsw(i-1, swdimension*(swdimension - 1)+i-1);
   }
     
}

void leafspine() {

//  6 7 8 9 10 11 
//  0 1 2 3 4  5  

   switches.Create(12);
   network.Add(switches);

   TimpHelper timpRouting;
   ContraHelper contraRouting;
   DashHelper dashRouting;
   Ipv4ListRoutingHelper listRH;
   if (routing == ROUTING_TIMP)
      listRH.Add (timpRouting, 0);
   if (routing == ROUTING_CONTRA)
      listRH.Add (contraRouting, 0);
   if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
      listRH.Add (dashRouting, 0);


   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp){
      if (routing == ROUTING_TIMP)
         internet.SetRoutingHelper (listRH);
      if (routing == ROUTING_CONTRA)
         internet.SetRoutingHelper (listRH); 
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
         internet.SetRoutingHelper (listRH);   
   }
   internet.Install (switches);
   
   for (unsigned long i = 0; i < 6; i++) 
     for (unsigned long j = 6; j < 12; j++) {
      connectsw(i, j);
     }
     
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

void circle(unsigned int swcount) {

   switches.Create(swcount);
   network.Add(switches);

   TimpHelper timpRouting;
   ContraHelper contraRouting;
   DashHelper dashRouting;
   Ipv4ListRoutingHelper listRH;
   if (routing == ROUTING_TIMP)
      listRH.Add (timpRouting, 0);
   if (routing == ROUTING_CONTRA)
      listRH.Add (contraRouting, 0);
   if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
      listRH.Add (dashRouting, 0);


   InternetStackHelper internet;
   internet.SetIpv6StackInstall (false);
   if (!flowecmp){
      if (routing == ROUTING_TIMP)
         internet.SetRoutingHelper (listRH);
      if (routing == ROUTING_CONTRA)
         internet.SetRoutingHelper (listRH); 
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET)
         internet.SetRoutingHelper (listRH);   
   }
   internet.Install (switches);
     
   // first circle
   for (unsigned long i = 0; i < swcount - 2; i++) {

   unsigned long curlevel = i + 1;
   connectsw(i, curlevel);

   }

   // last in circle
   connectsw(swcount - 2, 0);

   // than central
   for (unsigned long i = 0; i < swcount - 1; i++) {	
       connectsw(swcount - 1, i);
   }

}

void addservers(int swservers, int torcount) {

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
			AddNetwork (p2pNodes.Get(0)->GetId(), addr1, "255.255.255.0");
		}

	}

}

void RxTrace(Ptr<const Packet> packet, const Address &address, const Address &localAddress) {
   InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (address);
   InetSocketAddress localIaddr = InetSocketAddress::ConvertFrom (localAddress);

   std::string flowkey = getflowid(localIaddr);
   std::pair<int,double> flowprops = flowsprops[flowkey];
   int flowsize = flowprops.first;
   double flowtime = flowprops.second;

   std::stringstream ss;
   ss << " RxTrace " << iaddr.GetIpv4() << ":" << iaddr.GetPort() << " local " << localIaddr.GetIpv4() << ":" << localIaddr.GetPort() << " remote " << iaddr.GetIpv4();
   std::string s = ss.str();
   if ( flows.find(s) == flows.end() ) 
      flows[s] = packet->GetSize();
   else
      flows[s] += packet->GetSize();
   packets[s]++;
   static int floword = 1;
   if (flows[s] >= flowsize) {
      Time duration = Simulator::Now () - Seconds (flowtime);    
      
      if (flowsize == 30000) 
         std::cout << "MEASURED flow " << floword++ << " finished, duration " << duration.GetMicroSeconds() << s << " " << Simulator::Now ().GetMicroSeconds() << " flows[s] " << flows[s] << " packets[s] " << packets[s] << std::endl;

      if (flowsize == 30000) {
         durationfile << " " <<  duration.GetMicroSeconds();
         flowplacefile << " " <<  flowplace++;
      }
   }
}

void printsetup() {

   int sw = switches.GetN();
   std::cout << "switches: ";
   for (int i = 0; i < sw; i++) 
			std::cout << switches.Get(i)->GetId() << " ";
	std::cout << std::endl;
	
	
	std::cout << "switchNeighbours: ";
	std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >>::iterator it;
	for (it = switchNeighbours.begin (); it != switchNeighbours.end (); it++)
    {
      std::cout << "switch " << it->first << ": ";
      std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator ita1;
      for (ita1 = it->second.begin (); ita1 != it->second.end (); ita1++) {
         std::cout << "( " << std::get<0>(*ita1) << ", " << std::get<1>(*ita1) << ", " << std::get<2>(*ita1) << " ), " ;
      }
      
    }
	std::cout << std::endl;
	
	std::cout << "switchNetworks: ";
	std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >>::iterator ita;
   for (ita = switchNetworks.begin (); ita != switchNetworks.end (); ita++)
    {
      std::cout << "switch " << ita->first << ": ";
      std::list<std::pair <Ipv4Address, Ipv4Mask>>::iterator ita1;
      for (ita1 = ita->second.begin (); ita1 != ita->second.end (); ita1++) {
         std::cout << "( " << ita1->first << ", " << ita1->second << " ), " ;
      }
      
    }
	std::cout << std::endl;
}

uint32_t mIPv4Drop = 0;

void IPv4Drop(const Ipv4Header &header, Ptr< const Packet > packet, Ipv4L3Protocol::DropReason reason, Ptr< Ipv4 > ipv4, uint32_t interface)
{
    mIPv4Drop++;
    std::cout << " drop packet " << reason << "  " << " " << Simulator::Now ().GetMicroSeconds() << std::endl;
}

void scheduleFlows(int servercount, double flowinterarrival) {

   Ptr<ExponentialRandomVariable>exponential=CreateObject<ExponentialRandomVariable>();
   exponential->SetAttribute("Bound", DoubleValue (10000));
   exponential->SetAttribute("Mean", DoubleValue (flowinterarrival)); 

   Ptr<UniformRandomVariable> uniformsw = CreateObject<UniformRandomVariable> ();
   uniformsw->SetAttribute ("Min", DoubleValue (0));
   uniformsw->SetAttribute ("Max", DoubleValue (servercount)); 

   int flow_size[] = {5000, 7000, 10000, 20000, 30001, 50000, 100000, 500000};
   int till[] = {50, 60, 70, 80, 90, 95, 99, 100};
   
   Ptr<UniformRandomVariable> uniformfs = CreateObject<UniformRandomVariable> ();
   uniformfs->SetAttribute ("Min", DoubleValue (0));
   uniformfs->SetAttribute ("Max", DoubleValue (100)); // use floor function for rounding
   
   // add flows until sim end
   double simtime = 0;
   int serverport = 9000;
   
   while (simtime < SIM_DURATION) {
      // uniform switch selection
      double x = uniformsw->GetValue ();
      int serverhost = static_cast <int> (std::floor(x));
      x = uniformsw->GetValue ();
      int clienthost = static_cast <int> (std::floor(x));
      if (serverhost == clienthost)
         continue;
      
      // exponential interarival time selection
      double interarival = exponential->GetValue ();
      simtime = simtime + interarival;
      
      if (simtime >= SIM_DURATION)
         break;
         
      if (simtime < FLOWS_START_BACKGROUND)
         continue;
      
      // flow size selection from real world distribution
      x = uniformfs->GetValue ();
      int fs = static_cast <int> (std::floor(x));
      int flowsize = 0;
      for (int i = 0; i < 8; i++)
         if (fs<=till[i]) {
            flowsize = flow_size[i];
            break;
         }
      if (flowsize == 0) {
      	flowsize = 10000;
         std::cout << "missed flow size" << std::endl;
      }
      

      std::cout << "add flow server " << serverhost << " client " << clienthost << " simtime " << simtime << " flowsize " << flowsize << std::endl;  
      addtcpserver(serverhost, servers, serverport);
      addtcpclient(serverhost, clienthost, servers, serverport, simtime, flowsize);
      serverport++;
      
   }
   
}

int main (int argc, char **argv)
{
  
   int oldicount = 0;

   int topology = 0;

   int backgroundtraffic = 1;

   CommandLine cmd;
   cmd.AddValue ("flowecmp", "Flow based ECMP or TIMP/Contra", flowecmp);
   cmd.AddValue ("oldicount", "Number of OLDI-s executing simultaneously", oldicount);
   cmd.AddValue ("routing", "1 - timp, 2 - contra, 3 - dash packet, 4 - dash flowlet", routing);
   cmd.AddValue ("topology", "1 - hypercube, 2 - flattened butterfly, 3 - torus, 4-circle, 5-leafspine", topology);
   cmd.AddValue ("backgroundtraffic", "1 - with background traffic, 2 - without background traffic", backgroundtraffic);

   cmd.Parse (argc, argv);

   remove( "routes1.txt" );

   remove( "flowduration.txt" );
   remove( "flowcount.txt" );
   
   remove( "ttl.txt" );

   int swcount;
   int swdimension;
   
   if (topology == 1) {
      swcount = 16;
   }
   if (topology == 2) {
      swcount = 8;
   }
   if (topology == 3) {
      swdimension = 4;
      swcount = swdimension*swdimension;
   }
   if (topology == 4) {
      swcount = 15;
      
    }
   int devcount = (SWSERVERS+1)*swcount;
   int servercount = devcount - swcount;
   
   if (topology == 5) {
      swcount = 12;
      servercount = 6*SWSERVERS;
      devcount = swcount + servercount;
   }
   
    
   
   
   std::cout << "devcount " << devcount << std::endl;
   links = new Links(0, swcount);
   nodes = new Nodes(swcount);
   
   int maxlen = 15;
   
   double expmean = 3e-6;
   
   if (topology == 1) {
        hypercube();
        expmean = 3.125e-6;
   }

   if (topology == 2) {
      butterfly(3);
      expmean = 6.25e-6;
   }
      
   if (topology == 3) {
      expmean = 3.125e-6;
      torus(swdimension);
   }
   
   if (topology == 4) {
      circle(swcount);
   }

   if (topology == 5) {
      leafspine();
      expmean = 8.333e-6;
   }

	// add servers
	addservers(SWSERVERS, swcount); 

   

   std::list< int > torlist;
   for (int i = 0; i < swcount; i++)
      torlist.push_back(i);  

  
   TimpHelper timpRouting;
   ContraHelper contraRouting;
   DashHelper dashRouting;

   if (!flowecmp) {
      if (routing == ROUTING_TIMP) {
         nodes->topologyfinished();
         nodes->nDepthFirstSearch(links, maxlen);
         controller = timpRouting.GetTimpGlobal ();
         printsetup();
         controller->Initialize(network, torlist, switchNeighbours, switchNetworks, switches);
      } else {
         if (routing == ROUTING_CONTRA) {
            contraGlobal = contraRouting.GetContraGlobal ();
            printsetup();
            contraGlobal->Initialize(switches, torlist, switchNeighbours, switchNetworks, PROBE_PERIOD);
         } else {
            
              if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET) {
                  dashGlobal = dashRouting.GetDashGlobal ();
                  printsetup();
                  if (routing == ROUTING_DASH_PACKET)
                     dashGlobal->Initialize(switches, torlist, switchNeighbours, switchNetworks, PROBE_PERIOD, true, topology);
                  else
                     dashGlobal->Initialize(switches, torlist, switchNeighbours, switchNetworks, PROBE_PERIOD, false, topology);
               }
         }
      }
      
   } else {
      Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting",BooleanValue(true));
	   Ipv4GlobalRoutingHelper::PopulateRoutingTables();
   }

   // default routes for servers
   if (!flowecmp) {
      Ptr<Ipv4StaticRouting> staticRouting;
      char adr[100];
      for (int i = 0; i < servercount; i++) {
         staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (servers.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
         sprintf(adr, "10.1.%i.1", i+29);   
         staticRouting->SetDefaultRoute (Ipv4Address (adr), 1 ); 
      }
   }

   NS_LOG_INFO ("Create Applications.");

   std::cout << " swcount " << swcount << std::endl;

   std::cout << " scheduleFlows " << swcount << std::endl;
   
   if (backgroundtraffic == 1)
      scheduleFlows(servercount, expmean);

   int flownum = 1;
   
   if (topology == 5) {
      int swid = 0;
      for (int x = 0; x < oldicount; x++) {
	       int y = swid % SWSERVERS;
	       addtcpserver(y, servers, 9000+x);
          addtcpclient(y, 1 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES); 
          addtcpclient(y, 2 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 3 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 4 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 5 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 1 * SWSERVERS + (y+15) % SWSERVERS , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 2 * SWSERVERS + (y+15) % SWSERVERS , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 3 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES); 
          addtcpclient(y, 4 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 5 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES);
	       swid++;
	       std::cout << "add partition-aggregate " << flownum++ << std::endl; 
      }
      
   } 
   
   if (topology == 2) {
      int swid = 0;
      for (int x = 0; x < oldicount; x++) {

	       int y = swid % SWSERVERS;
	       addtcpserver(y, servers, 9000+x);
	       
          addtcpclient(y, 1 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES); 
          addtcpclient(y, 2 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 3 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 4 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 5 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 6 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 7 * SWSERVERS + y , servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 1 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES); 
          addtcpclient(y, 3 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 5 * SWSERVERS + (y+15) % SWSERVERS, servers, 9000+x, FLOWS_START, FLOW_BYTES);
	       swid++;
	       std::cout << "add partition-aggregate " << flownum++ << std::endl; 
      }
      
   } 
   
   if (topology != 2 && topology != 5) { 
      int swid = 1;
      for (int x = 0; x < oldicount; x++) {
	       int y = swid % SWSERVERS;
	       addtcpserver(y, servers, 9000+x);

          addtcpclient(y, 3 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 4 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 5 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 6 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 7 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 8 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 9 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 10 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 11 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          addtcpclient(y, 12 * SWSERVERS + y, servers, 9000+x, FLOWS_START, FLOW_BYTES);
          swid++;
          std::cout << "add partition-aggregate " << flownum++ << std::endl; 
       }   
      
   }
   

   durationfile.open ("flowduration.txt", std::ios::out | std::ios::app);
   flowplacefile.open ("flowcount.txt", std::ios::out | std::ios::app);

   AsciiTraceHelper ascii;
   
   Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback(&RxTrace));
   Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/Drop" , MakeCallback(&IPv4Drop));

   NS_LOG_INFO ("Run Simulation.");
   if (flowecmp) 
      Simulator::Stop (Seconds (SIM_DURATION_ECMP));
   else
      Simulator::Stop (Seconds (SIM_DURATION));
   Simulator::Run ();

   if (!flowecmp) {
      if (routing == ROUTING_TIMP) 
         controller->PrintStatistics();
      if (routing == ROUTING_CONTRA) 
         contraGlobal->PrintStatistics();
      if (routing == ROUTING_DASH_PACKET || routing == ROUTING_DASH_FLOWLET) 
         dashGlobal->PrintStatistics();
   }

   Simulator::Destroy ();
   
   durationfile.flush();
   flowplacefile.flush();

   durationfile.close();
   flowplacefile.close();
         
   std::cout << "mIPv4Drop " << mIPv4Drop << std::endl;

   NS_LOG_INFO ("Done.");
}

