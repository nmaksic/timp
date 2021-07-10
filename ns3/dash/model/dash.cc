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
// Author: Natasa Maksic <maksicn@etf.rs>
//


#include "dash.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/dash-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/loopback-net-device.h"
#include "ns3/packet.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/core-module.h"
#include <algorithm>
#include <climits>
#include <math.h>

#define DASH_PORT 11001

#define FLOWLET_TIMEOUT 200

#define TOPOLOGY_LEAFSPINE 5

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Dash");

NS_OBJECT_ENSURE_REGISTERED (Dash);

Dash::Dash ()
  : m_ipv4 (0), m_initialized (false) {
  m_routes.clear();
}

Dash::~Dash () {
}

TypeId
Dash::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::Dash")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<Dash> ()
    ;
  return tid;
}


void Dash::DoInitialize () {

   m_initialized = true;

   m_probeVersionCounter = 0;

   for (uint32_t i = 0 ; i < m_ipv4->GetNInterfaces (); i++) {
      if (DynamicCast<LoopbackNetDevice> (m_ipv4->GetNetDevice (i)))
         continue;

      m_ipv4->SetForwarding (i, true);

      
   }

   uniformpath = CreateObject<UniformRandomVariable> ();
   uniformpath->SetAttribute ("Min", DoubleValue (0));
   uniformpath->SetAttribute ("Max", DoubleValue (1));

   Ipv4RoutingProtocol::DoInitialize ();
   
   
}

// generate probe and send to all neighbours
void Dash::GenerateProbe ()
{

   m_probeVersionCounter++;
   if (m_probeVersionCounter == UINT64_MAX)
      m_probeVersionCounter = 0;
   Ipv4Address localAdr("127.0.0.1");
   uint32_t origin = m_ipv4->GetObject<Node> ()->GetId ();
   uint64_t mv = 0;
   uint32_t pid = 0;

   sendcost(origin, mv, pid, localAdr, -1, m_probeVersionCounter);

   Simulator::Schedule (Seconds (m_probePeriod), &Dash::GenerateProbe, this);
}

Ptr<Ipv4Route> Dash::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{

   Ipv4Address destination = header.GetDestination ();
   Ptr<Ipv4Route> rtentry = 0;

   rtentry = LookupDash (destination, p, header, oif, true );

   if (rtentry)
      sockerr = Socket::ERROR_NOTERROR;
   else
      sockerr = Socket::ERROR_NOROUTETOHOST;

   return rtentry;
}

bool Dash::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                        UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                        LocalDeliverCallback lcb, ErrorCallback ecb) {

  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  Ipv4Address dst = header.GetDestination ();

   if (m_ipv4->IsDestinationAddress (dst, iif))
   {
      if (!lcb.IsNull ()) {
          lcb (p, header, iif);
          return true;
      } else
          return false;
   }

   if (dst.IsMulticast ())
      return false; 

   if (header.GetDestination ().IsBroadcast ())
   {
      if (!ecb.IsNull ())
         ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return false;
   }

   if (m_ipv4->IsForwarding (iif) == false)
   {
      if (!ecb.IsNull ())
         ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return true;
   }

  Packet packet = *p;
  Ptr<Packet> ptr(&packet);
  Ptr<Ipv4Route> rtentry = LookupDash (header.GetDestination (), ptr, header, 0, false  );

  if (rtentry != 0) {
      ucb (rtentry, p, header);  
      return true;
  } else
      return false; 
}

void Dash::NotifyInterfaceUp (uint32_t i) {
  NS_LOG_FUNCTION (this << i);

  if (DynamicCast<LoopbackNetDevice> (m_ipv4->GetNetDevice (i)))
     return;

  if (!m_initialized)
     return;


  bool sendSocketFound = false;
  for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) 
   if (iter->second == i) {
      sendSocketFound = true;
      break;
   }


  for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++) {
      Ipv4InterfaceAddress address = m_ipv4->GetAddress (i, j);

      if (address.GetScope() != Ipv4InterfaceAddress::HOST && sendSocketFound == false) {

          TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
          Ptr<Node> theNode = GetObject<Node> ();
          Ptr<Socket> socket = Socket::CreateSocket (theNode, tid);
          InetSocketAddress local = InetSocketAddress (address.GetLocal (), DASH_PORT);
          socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
          socket->Bind (local);
          socket->SetIpRecvTtl (true);
          m_sendSocketList[socket] = i;
          socket->SetRecvCallback (MakeCallback (&Dash::Receive, this));
          socket->SetRecvPktInfo (true);
       }

  }


}

void Dash::NotifyInterfaceDown (uint32_t interface) {

   for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) {
      if (iter->second == interface) {
         iter->first->Close ();
         m_sendSocketList.erase (iter);
         break;
      }
   }

}

void Dash::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Dash::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Dash::SetIpv4 (Ptr<Ipv4> ipv4) {
  NS_LOG_FUNCTION (this << ipv4);

  uint32_t i = 0;
  m_ipv4 = ipv4;

   for (i = 0; i < m_ipv4->GetNInterfaces (); i++)
      if (m_ipv4->IsUp (i))
         NotifyInterfaceUp (i);
      else
         NotifyInterfaceDown (i);

}

void Dash::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
}

void Dash::DoDispose () {
  NS_LOG_FUNCTION (this);

  for (RoutesI j = m_routes.begin ();  j != m_routes.end (); j = m_routes.erase (j))
    {
      delete j->first;
    }
  m_routes.clear ();


  for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ )
    {
      iter->first->Close ();
    }
  m_sendSocketList.clear ();

  m_ipv4 = 0;

  Ipv4RoutingProtocol::DoDispose ();
}


void Dash::PrintPrices(bool alternateonly, int routercount) {

}

void Dash::PrintStatistics() {

  
   std::cout << "ROUTER " << m_ipv4->GetObject<Node> ()->GetId () << ": " << m_ipv4->GetAddress (1, 0) << std::endl;

   for (int i = 0; i < PRT_MAX; i++)
      if (portflows[i] != 0) 
         std::cout << " Port " << i << " flows " << portflows[i] << std::endl;   


    for (int i = 0; i < PRT_MAX; i++)
      if (porthashes[i] != 0) 
         std::cout << " Port " << i << " porthashes " << porthashes[i] << std::endl;
	
}


Ptr<Ipv4Route> Dash::LookupDash (Ipv4Address dst, Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> interface, bool generatedpacket) {

   Ptr<Ipv4Route> rtentry = 0;


   rtentry = Create<Ipv4Route> ();
   Ipv4Address gw("0.0.0.0");
   rtentry->SetGateway (gw);
   
   if (generatedpacket) {
   
      DashTag tag;
      p->PeekPacketTag (tag);
      
      int ui = tag.GetSimpleValue();
      if (ui == 1) {
   
         // route to interface towards neighbor based on dst address
         std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
         for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it)
            if (std::get<1>(*it) == dst) 
               rtentry->SetOutputDevice (m_ipv4->GetNetDevice (std::get<0>(*it)));
         
         
         return rtentry;
         
      }
           
   }
   
   // check server interfaces
   for (uint32_t i = 0 ; i < m_ipv4->GetNInterfaces (); i++) {
      std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
         bool isServerIfc = true;
         for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
            if (std::get<0>(*it) == i) {
               isServerIfc = false;
               break;
            }
         }
         if (isServerIfc) {
            for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++) {
               Ipv4InterfaceAddress address = m_ipv4->GetAddress (i, j);
               Ipv4Address adr = address.GetLocal ();
               Ipv4Mask mask = address.GetMask ();
               
               if (mask.IsMatch (dst, adr)) {
               
                  rtentry->SetOutputDevice (m_ipv4->GetNetDevice (i));
                  return rtentry;
               }
            }  
         }
     }

   
   int dstid = getdstid(dst);
   
   if (dstid == -1)
      return NULL;
   
   uint32_t localTag;
   uint32_t pid;
   
   KeyFwdT key;
   
   // check if packet from host to check bestT   
   if (fromHost(interface)) {
       localTag = m_ipv4->GetObject<Node> ()->GetId ();
  	    pid = 0;
        key = std::make_tuple (dstid, localTag, pid);
   } else {
        DashPacketTag rcvPacketTag;
        p->RemovePacketTag (rcvPacketTag);
        localTag = rcvPacketTag.GetTag();
        localTag = m_ipv4->GetObject<Node> ()->GetId ();
  	     pid = rcvPacketTag.GetPid();
   	  key = std::make_tuple (dstid, localTag, pid);
   }
 	
   // date center case, single policy, single entry per flowlet, dash paper chapter 5.3    
       
   int fh = CalcFlowHash (p, header);

   int64_t curtime = ns3::Simulator::Now().GetMicroSeconds ();

   int outputifc = flowhash[dstid * DASH_HASH_SIZE + fh];
   int64_t elapsedtime = flowtime[dstid * DASH_HASH_SIZE + fh];

   // check ttl, check loops according to Dash paper
   int ttl = header.GetTtl();
   
   if (ttl < 35) {
      std::pair<int, uint32_t> bi = boundariesIndex(fh, (uint32_t)dstid);
      // loop detected, drop this path if more than one path present
      if (bi.first != -1)
         if (fwdMapT[(uint32_t)dstid].size() > 1)
            fwdMapT[(uint32_t)dstid].erase(bi.second);
      std::cout << "looping  " << " dstid " << dstid << std::endl;
   }
   // flowlet gap is set as 200us, as in Contra paper,
   // it is fine upper limit of maximal delay difference between two paths
   // in tested topologies
   if (curtime < elapsedtime + FLOWLET_TIMEOUT && outputifc > 0 && ttl > 10 && !m_packetRouting) {
      
      uint32_t interfaceIdx = outputifc;
      
      if (interfaceIdx == 0){
	   std::cout << "selectedoutputifc 0" << std::endl;
      }
      
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      flowtime[dstid * DASH_HASH_SIZE + fh] = curtime;

      porthashes[interfaceIdx]++;

      UpdatePortUtil(outputifc, p->GetSize());
      
      uint32_t nbrtag = getNbrTag(interfaceIdx);
      
      DashPacketTag dashPacketTag;
	   dashPacketTag.SetPid (pid);
	   dashPacketTag.SetTag (nbrtag);
	   p->ReplacePacketTag (dashPacketTag);
	   
   } else {
      // empty previous hash
      if (elapsedtime > 0) {
         flowtime[dstid * DASH_HASH_SIZE + fh] = 0;
      }
      
      // choose next hop
      int selectedoutputifc = 0;


     
      std::pair<int, uint32_t> bi = boundariesIndex(fh, (uint32_t)dstid); 
      
      if (bi.first != -1) {
         ValueFwdT val = fwdMapT[(uint32_t)dstid][bi.second];
         
         uint32_t nexttag = std::get<1>(val);
        	uint32_t nexthop = std::get<2>(val);

         // update tag
         DashPacketTag dashPacketTag;
         dashPacketTag.SetPid (pid);
         dashPacketTag.SetTag (nexttag);
         p->ReplacePacketTag (dashPacketTag);
         
         selectedoutputifc  = nexthop; 
         
      } else
         std::cout << "forwarding table miss" << std::endl;

     
      // write hash
      if (selectedoutputifc > 0) {
            rtentry->SetOutputDevice (m_ipv4->GetNetDevice (selectedoutputifc));
            flowhash[dstid * DASH_HASH_SIZE + fh] = selectedoutputifc;
            flowtime[dstid * DASH_HASH_SIZE + fh] = curtime;
	         UpdatePortUtil(selectedoutputifc, p->GetSize());
      }else {
	      std::cout << "selectedoutputifc 0" << std::endl;
      }
      
   }

  return rtentry;

}

std::pair<int, uint32_t> Dash::boundariesIndex(int flowhash, uint32_t origin) {
   if (fwdMapT.find(origin) != fwdMapT.end()) {

   
      std::map<uint32_t, ValueFwdT> m1 = fwdMapT[origin]; // origin, port, forwarding entry
      
      // iterate map, generate bounds and select outputifc
      std::map<uint32_t, ValueFwdT>::iterator it;
      
      // get sum of values
      double summv = 0;
      for (it = m1.begin (); it != m1.end (); it++) {
         ValueFwdT val = it->second;
         uint64_t mv = std::get<0>(val);
         if (mv == 0)
            mv = 1;
         summv += 1.0/mv;
      }
      
      double sum1 = 0;
      int counter = 0;
      for (it = m1.begin (); it != m1.end (); it++) { 
         ValueFwdT val = it->second;
         uint64_t mv = std::get<0>(val);
         if (mv == 0)
            mv = 1;
         sum1 += 1.0/mv;
         counter++;
         
         double x = uniformpath->GetValue (); 
         
         if (sum1/summv >= x) {
            
            return std::pair<int, uint32_t>(counter, it->first);
         }

      }
      
   }
   
   return std::pair<int, uint32_t>(-1, 0);

}

bool Dash::fromHost(Ptr<NetDevice> interface) {

	uint32_t ipInterfaceIndex = m_ipv4->GetInterfaceForDevice (interface);

         bool isServerIfc = true;
         std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
         for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
            if (std::get<0>(*it) == ipInterfaceIndex) {
               isServerIfc = false;
               break;
            }
         }
         
	return isServerIfc;

}


int Dash::getdstid(ns3::Ipv4Address& dst) {
   int result = -1;
   uint16_t longestMask = 0;

   std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >>::iterator ita;
   for (ita = switchNetworks.begin (); ita != switchNetworks.end (); ita++)
    {
      std::list<std::pair <Ipv4Address, Ipv4Mask>>::iterator ita1;
      for (ita1 = ita->second.begin (); ita1 != ita->second.end (); ita1++) {
         Ipv4Address adr = ita1->first;
         Ipv4Mask mask = ita1->second;
         uint16_t maskLen = mask.GetPrefixLength ();
         if (mask.IsMatch (dst, adr)) {
         
            if (maskLen < longestMask)
                     continue;

            longestMask = maskLen;
            
            result = ita->first;
             
         }
      }
    }

    if (result == -1)
      std::cout << "Dash::getdstid() error" << std::endl;
      
    return result;

}

void Dash::sendcost(uint32_t origin, uint64_t mv, uint32_t pid, Ipv4Address senderAddress, uint32_t incomingInterface, uint64_t version) { 

   NS_LOG_FUNCTION (this << origin << mv << pid);

   if (m_topology == TOPOLOGY_LEAFSPINE && origin > 5)
      return;

   std::map<int, std::map<int, uint64_t>>::iterator it1 = lastProbeVersion.find(origin); // dst, port, lastprobe
   
   std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;  // // port id, neigbout adr, neigbour id
      for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){   
          uint32_t ifc = std::get<0>(*it);
          uint64_t portProbeId = 0;
          if (it1 != lastProbeVersion.end()) {
            std::map<int, uint64_t> rcvprobes =  lastProbeVersion[origin];
            std::map<int, uint64_t>::iterator it2 = rcvprobes.find(ifc);
            if (it2 != rcvprobes.end())
               portProbeId = rcvprobes[ifc];
          }
          
          bool checkReceived = false;
          if (fwdMapT.find(origin) != fwdMapT.end()) {
            std::map<uint32_t, ValueFwdT> m1 = fwdMapT[origin];
            if (m1.find(std::get<0>(*it)) != m1.end()) {
               checkReceived = true;
            }
         }
          
         std::vector<uint32_t> fwdlist = updateforward[origin];
         
         if ( find(fwdlist.begin(), fwdlist.end(), std::get<2>(*it)) != fwdlist.end() ) {
          

          
             if (std::get<0>(*it) != incomingInterface && version > portProbeId && !checkReceived) {
                 // find socket and send
                 Ptr<Socket> sendingSocket;
                 for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) {
                        if (std::get<0>(*it) == iter->second) { 
                            sendingSocket = iter->first;
                            sendcost2(sendingSocket, origin, mv, pid, std::get<1>(*it), std::get<2>(*it), version); 
                            
                        } 
                     }
	            }
	      }
      }
}

void Dash::sendcost2(Ptr<Socket> sendingSocket, uint32_t origin, uint64_t mv, uint32_t pid, const Ipv4Address &dstAdr, uint32_t tag, uint64_t version) {

  Ptr<Packet> p = Create<Packet> ();
  SocketIpTtlTag ttltag;
  p->RemovePacketTag (ttltag);
  ttltag.SetTtl (1);
  p->AddPacketTag (ttltag);

  DashTag dashTag;
  dashTag.SetSimpleValue (1);
  p->AddPacketTag (dashTag);

  DashUpdateHeader hdr;
  hdr.SetTag (tag);
  hdr.SetOrigin (origin);
  hdr.SetMv (mv);
  hdr.SetPid (pid);
  hdr.SetVersion (version);

  p->AddHeader (hdr);

  sendingSocket->SendTo (p, 0, InetSocketAddress (dstAdr, DASH_PORT));

}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

int Dash::CalcFlowHash (Ptr<const Packet> p, const Ipv4Header &header) {

   uint32_t src=	header.GetSource().Get();
   uint32_t dst=	header.GetDestination().Get();
   uint8_t protocol = header.GetProtocol ();

   uint16_t dstport = 0;
   uint16_t srcport = 0;
   if (header.GetProtocol() == 6) {
      TcpHeader tcpHeader;
      p->PeekHeader(tcpHeader);
      dstport = tcpHeader.GetDestinationPort ();
      srcport = tcpHeader.GetSourcePort ();
   } else {
      if (header.GetProtocol() == 17) {
         UdpHeader udpHeader;
         p->PeekHeader(udpHeader);
         dstport = udpHeader.GetDestinationPort ();
         srcport = udpHeader.GetSourcePort ();
      } 
   }

   std::size_t h1 = 0;

   hash_combine<uint32_t>(h1, src);
   hash_combine<uint32_t>(h1, dst);
   hash_combine<uint8_t>(h1, protocol);
   hash_combine<uint16_t>(h1, dstport);
   hash_combine<uint16_t>(h1, srcport);

   return h1 % DASH_HASH_SIZE; // prime number

}

void Dash::SetNeighbors(std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> neighbours) {
   switchNeighbours = neighbours;
   
   std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
   for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
   
      uint32_t i = std::get<0>(*it); // it->first;
      
      for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++) {
            Ipv4InterfaceAddress address = m_ipv4->GetAddress (i, j);
            if (address.GetScope() != Ipv4InterfaceAddress::HOST) {

               TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
               Ptr<Node> theNode = GetObject<Node> ();
               Ptr<Socket> socket = Socket::CreateSocket (theNode, tid);
               InetSocketAddress local = InetSocketAddress (address.GetLocal (), DASH_PORT);
               socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
               socket->Bind (local);
               socket->SetIpRecvTtl (true);
               m_sendSocketList[socket] = i;
               socket->SetRecvCallback (MakeCallback (&Dash::Receive, this));
               socket->SetRecvPktInfo (true);

            }
       }
   }
   // schedule probe sending
   Simulator::Schedule (Seconds (m_probePeriod), &Dash::GenerateProbe, this);
}

void Dash::SetDestinationNetworks(std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> networks, int topology) {
  switchNetworks = networks;
  m_topology = topology;
  
  uint32_t device = m_ipv4->GetObject<Node> ()->GetId ();  
  
  std::cout << " SetDestinationNetworks " << device << std::endl;
  
  if (topology == TOPOLOGY_LEAFSPINE) {
      if (device >= 0 && device < 6) {
         updateforward[device] = {6,7,8,9,10,11};
         for (uint32_t i = 0; i < 6; i++)
            if (i != device) 
               updateforward[i] = {};
      }
      
      if (device >= 6 && device < 12) {
          updateforward[0] = {1,2,3,4,5};
          updateforward[1] = {0,2,3,4,5};
          updateforward[2] = {0,1,3,4,5};
          updateforward[3] = {0,1,2,4,5};
          updateforward[4] = {0,1,2,3,5};
          updateforward[5] = {0,1,2,3,4};
      }
   
  }
  
}


void Dash::Receive (Ptr<Socket> socket) {

   Address sender;
   Ptr<Packet> packet = socket->RecvFrom (sender);
   InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (sender);

   Ipv4Address senderAddress = senderAddr.GetIpv4 ();
   uint16_t senderPort = senderAddr.GetPort ();

   Ipv4PacketInfoTag interfaceInfo;
   if (!packet->RemovePacketTag (interfaceInfo))
      NS_ABORT_MSG ("Error DASH message.");

   uint32_t incomingIf = interfaceInfo.GetRecvIf ();
   Ptr<Node> node = this->GetObject<Node> ();
   Ptr<NetDevice> dev = node->GetDevice (incomingIf);
   uint32_t ipInterfaceIndex = m_ipv4->GetInterfaceForDevice (dev);

   DashUpdateHeader hdr;
   packet->RemoveHeader (hdr);

   HandleUpdate(hdr, senderAddress, senderPort, ipInterfaceIndex); 

  return;

}

void Dash::HandleUpdate (DashUpdateHeader hdr, Ipv4Address senderAddress, uint16_t senderPort, uint32_t incomingInterface) {


  uint32_t origin = hdr.GetOrigin ();
  uint32_t pid = hdr.GetPid ();
  uint32_t tag = hdr.GetTag ();
  uint64_t mv = hdr.GetMv ();
  uint64_t version = hdr.GetVersion ();
  uint64_t updatedMv = getUpdatedMv(mv, incomingInterface);
  
  uint32_t device = m_ipv4->GetObject<Node> ()->GetId ();  
  
  if (origin == device)
      return;

  bool firstnewprobe = false;
  if ( lastProbeVer.find(origin) == lastProbeVer.end() ) {
        lastProbeVer.insert ( std::pair<int,uint64_t>(origin,version) );
        firstnewprobe = true;
   } else {
      uint64_t lastProbe = lastProbeVer[origin];
      if (version != lastProbe) {
         firstnewprobe = true;
      }
      lastProbeVer[origin] = version; 
   }
  
   // drop probe if already received on this port
   if ( lastProbeVersion.find(origin) == lastProbeVersion.end() ) {
        std::map<int,uint64_t> m; 
        m[incomingInterface] = version;
        lastProbeVersion.insert ( std::pair<int,std::map<int,uint64_t>>(origin,m) );
        
   } else {

      
      if ( lastProbeVersion[origin].find(incomingInterface) == lastProbeVersion[origin].end() ) {
      
         lastProbeVersion[origin].insert ( std::pair<int,uint64_t>(incomingInterface,version) );
      } else {
   
         
         uint64_t lastProbe = lastProbeVersion[origin][incomingInterface];
         
         if (version <= lastProbe || (version > UINT64_MAX-1000 && lastProbe < 1000)) {
            return; // drop update 
         }
      
         lastProbeVersion[origin][incomingInterface] = version; 
         
      }
   }
   
   // drop probe if newer probe received on any port
   if ( lastProbeId.find(origin) == lastProbeId.end() ) {
      lastProbeId.insert(std::pair<int,uint32_t>(origin,version));
   } else {
   
      uint32_t lastid = lastProbeId[origin];
      
      if (version < lastid || (version > UINT64_MAX-1000 && lastid < 1000)) {
            return; 
      }
      if (version > lastid) {
         lastProbeId[origin] = version;
      }
   }

  // process update

  uint32_t localTag = m_ipv4->GetObject<Node> ()->GetId ();
  if (tag != localTag) {
	std::cout << "bad tag" << std::endl;
  }

  uint32_t nbrtag = getNbrTag(incomingInterface);
  



  ValueFwdT newvalue = std::make_tuple (updatedMv, nbrtag, incomingInterface);
  if (nbrtag != UINT_MAX) {

      if (firstnewprobe) {
			
			if (fwdMapT.find(origin) != fwdMapT.end()) {
			   fwdMapT[origin].clear();
			} else
			   fwdMapT[origin] = std::map<uint32_t, ValueFwdT>();
			
			fwdMapT[origin][incomingInterface] = newvalue;
      } else {
      
         if (fwdMapT.find(origin) != fwdMapT.end()) {
            std::map<uint32_t, ValueFwdT> m1 = fwdMapT[origin];
            if (m1.find(incomingInterface) != m1.end()) {
               ValueFwdT v = m1[incomingInterface];
               uint64_t mv = std::get<0>(v);
               if (mv > updatedMv)
                  fwdMapT[origin][incomingInterface] = newvalue;
            } else
               fwdMapT[origin][incomingInterface] = newvalue;
         }
      }

      Ipv4Address nbradr = getNbrAdr(incomingInterface);
      uint32_t bestMv = updatedMv; 
      sendcost(origin, bestMv, pid, nbradr, incomingInterface, version);
   } else {
	   std::cout << "getNbrTag error " << std::endl;
   }  


}

uint64_t Dash::getUpdatedMv(uint64_t rcvMv, uint32_t incomingInterface) {

  uint64_t localMv = GetPortUtil(incomingInterface);

  return std::max(localMv, rcvMv);

}

uint32_t Dash::getNbrTag(uint32_t incomingInterface) {

  std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
  for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
 
    if (std::get<0>(*it) == incomingInterface) { //if (it->second == dst) {
       return std::get<2>(*it);
    }
  }
  std::cout << "getNbrTag failed " << std::endl;
  return UINT_MAX;
}

Ipv4Address Dash::getNbrAdr(uint32_t incomingInterface) {

  std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
  for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
 
    if (std::get<0>(*it) == incomingInterface) { //if (it->second == dst) {
       return std::get<1>(*it);
    }
       
  }
  std::cout << "getNbrTag failed " << std::endl;
  Ipv4Address adr("0.0.0.0");
  return adr;
}


void Dash::UpdatePortUtil(uint32_t port, uint32_t packetLen) {

   int64_t curtime = ns3::Simulator::Now().GetNanoSeconds ();
   

   if ( lastPacketTime.find(port) != lastPacketTime.end() ) {

	int64_t utilint = 0;
	int64_t lastPkt = lastPacketTime[port];

	if ( utilizationLocal.find(port) != utilizationLocal.end() ) {
		utilint = utilizationLocal[port];
	}	

	int64_t deltat = curtime - lastPkt; 

	double util = (double)packetLen + 1.0 * utilint / 100.0 * (1.0-1.0*deltat/m_tau);


	
	if (util <= 0)
	   util = 1; 

	utilizationLocal[port] = (uint64_t)round(util * 100);

   }

   lastPacketTime[port] = curtime;


}

int64_t Dash::GetPortUtil(uint32_t port) {

	int64_t util = 0;

	if ( utilizationLocal.find(port) != utilizationLocal.end() ) {
		util = utilizationLocal[port];
	}
	return util;	

}

void Dash::SetProbePeriod(double probePeriod) {
  m_probePeriod = probePeriod; // Dash paper proposes 0.5 * MAXRTT and more, set to few times RTT in simulations;
  std::cout << "m_probePeriod " << m_probePeriod << std::endl;
  m_tau = 0.5 * m_probePeriod * 1000000000;
}

void Dash::SetPacketRouting(bool packetRouting) {
  m_packetRouting = packetRouting; 
}

DashRoutingTableEntry::DashRoutingTableEntry (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface)
  : Ipv4RoutingTableEntry ( Ipv4RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, interface) ),
    m_status (DASH_INVALID), m_torId (0) {
}


void DashRoutingTableEntry::SetRouteStatus (Status_e status) {
   if (m_status != status)
      m_status = status;
}

DashRoutingTableEntry::Status_e DashRoutingTableEntry::GetRouteStatus (void) const {
   return m_status;
}

void DashRoutingTableEntry::SetDstId (int torId) {
  if (m_torId != torId)
      m_torId = torId;
}

int DashRoutingTableEntry::GetDstId () const {
  return m_torId;
}

}
