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


#include "contra.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/contra-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/loopback-net-device.h"
#include "ns3/packet.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include <algorithm>
#include <climits>
#include <math.h>

#define CONTRA_PORT 11001

#define FLOWLET_TIMEOUT 200

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Contra");

NS_OBJECT_ENSURE_REGISTERED (Contra);

Contra::Contra ()
  : m_ipv4 (0), m_initialized (false) {
  m_routes.clear();
}

Contra::~Contra () {
}

TypeId
Contra::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::Contra")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<Contra> ()
    ;
  return tid;
}


void Contra::DoInitialize () {

   m_initialized = true;

   m_probeVersionCounter = 0;

   for (uint32_t i = 0 ; i < m_ipv4->GetNInterfaces (); i++) {
      if (DynamicCast<LoopbackNetDevice> (m_ipv4->GetNetDevice (i)))
         continue;

      m_ipv4->SetForwarding (i, true);

      
   }

   Ipv4RoutingProtocol::DoInitialize ();
   
   
}

// generate probe and send to all neighbours
void Contra::GenerateProbe ()
{

   m_probeVersionCounter++;
   if (m_probeVersionCounter == UINT64_MAX)
      m_probeVersionCounter = 0;
   Ipv4Address localAdr("127.0.0.1");
   uint32_t origin = m_ipv4->GetObject<Node> ()->GetId ();
   uint64_t mv = 0;
   uint32_t pid = 0;
   
   sendcost(origin, mv, pid, localAdr, -1, m_probeVersionCounter);

   Simulator::Schedule (Seconds (m_probePeriod), &Contra::GenerateProbe, this);
}

Ptr<Ipv4Route> Contra::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{

   Ipv4Address destination = header.GetDestination ();
   Ptr<Ipv4Route> rtentry = 0;

   rtentry = LookupContra (destination, p, header, oif, true );

   if (rtentry)
      sockerr = Socket::ERROR_NOTERROR;
   else
      sockerr = Socket::ERROR_NOROUTETOHOST;

   return rtentry;
}

bool Contra::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
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
  Ptr<Ipv4Route> rtentry = LookupContra (header.GetDestination (), ptr, header, 0, false  );

  if (rtentry != 0) {
      ucb (rtentry, p, header);  
      return true;
  } else
      return false; 
}

void Contra::NotifyInterfaceUp (uint32_t i) {
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
          InetSocketAddress local = InetSocketAddress (address.GetLocal (), CONTRA_PORT);
          socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
          socket->Bind (local);
          socket->SetIpRecvTtl (true);
          m_sendSocketList[socket] = i;
          socket->SetRecvCallback (MakeCallback (&Contra::Receive, this));
          socket->SetRecvPktInfo (true);
       }

  }


}

void Contra::NotifyInterfaceDown (uint32_t interface) {

   for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) {
      if (iter->second == interface) {
         iter->first->Close ();
         m_sendSocketList.erase (iter);
         break;
      }
   }

}

void Contra::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Contra::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Contra::SetIpv4 (Ptr<Ipv4> ipv4) {
  NS_LOG_FUNCTION (this << ipv4);

  uint32_t i = 0;
  m_ipv4 = ipv4;

   for (i = 0; i < m_ipv4->GetNInterfaces (); i++)
      if (m_ipv4->IsUp (i))
         NotifyInterfaceUp (i);
      else
         NotifyInterfaceDown (i);

}

void Contra::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
}

void Contra::DoDispose () {
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


void Contra::PrintPrices(bool alternateonly, int routercount) {

}

void Contra::PrintStatistics() {

  
   std::cout << "ROUTER " << m_ipv4->GetObject<Node> ()->GetId () << ": " << m_ipv4->GetAddress (1, 0) << std::endl;

   for (int i = 0; i < PRT_MAX; i++)
      if (portflows[i] != 0) 
         std::cout << " Port " << i << " flows " << portflows[i] << std::endl;   


    for (int i = 0; i < PRT_MAX; i++)
      if (porthashes[i] != 0) 
         std::cout << " Port " << i << " porthashes " << porthashes[i] << std::endl;
	
}


Ptr<Ipv4Route> Contra::LookupContra (Ipv4Address dst, Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> interface, bool generatedpacket) {

   Ptr<Ipv4Route> rtentry = 0;


   rtentry = Create<Ipv4Route> ();
   Ipv4Address gw("0.0.0.0");
   rtentry->SetGateway (gw);
   
   if (generatedpacket) {
   
      ContraTag tag;
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
       localTag = m_ipv4->GetObject<Node> ()->GetId ();;
  	    pid = 0;
       if ( bestT.find(dstid) != bestT.end() ) {
	      key = bestT[dstid];
       } else {
	      std::cout << "BestT table miss" << std::endl;
         key = std::make_tuple (dstid, localTag, pid);
       }
   } else {
        ContraPacketTag rcvPacketTag;
        p->RemovePacketTag (rcvPacketTag);
        localTag = rcvPacketTag.GetTag();
        localTag = m_ipv4->GetObject<Node> ()->GetId ();
  	     pid = rcvPacketTag.GetPid();
        // setup key
   	  key = std::make_tuple (dstid, localTag, pid);
   }
 	
   // date center case, single policy, single entry per flowlet, contra paper chapter 5.3    
       
   int fh = CalcFlowHash (p, header);

   int64_t curtime = ns3::Simulator::Now().GetMicroSeconds ();

   int outputifc = flowhash[dstid * CONTRA_HASH_SIZE + fh];
   int64_t elapsedtime = flowtime[dstid * CONTRA_HASH_SIZE + fh];

   // check ttl, check loops according to Contra paper
   int ttl = header.GetTtl();

   // flowlet gap is set as 200us, as in Contra paper,
   // it is fine upper limit of maximal delay difference between two paths
   // in tested topologies
   if (curtime < elapsedtime + FLOWLET_TIMEOUT && outputifc > 0 && ttl > 10) {
      
      uint32_t interfaceIdx = outputifc;
      
      if (interfaceIdx == 0){
	   std::cout << "selectedoutputifc 0" << std::endl;
      }
      
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      flowtime[dstid * CONTRA_HASH_SIZE + fh] = curtime;

      porthashes[interfaceIdx]++;

      UpdatePortUtil(outputifc, p->GetSize());
      
      uint32_t nbrtag = getNbrTag(interfaceIdx);
      
      ContraPacketTag contraPacketTag;
	   contraPacketTag.SetPid (pid);
	   contraPacketTag.SetTag (nbrtag);
	   p->ReplacePacketTag (contraPacketTag);
	   
   } else {
      // empty previous hash
      if (elapsedtime > 0) {
         flowtime[dstid * CONTRA_HASH_SIZE + fh] = 0;
      }
      
      // choose next hop
      int selectedoutputifc = 0;

      if ( fwdT.find(key) != fwdT.end() ) {
	      ValueFwdT value = fwdT[key];

	      //uint64_t mv = std::get<0>(value);
	      uint32_t nexttag = std::get<1>(value);
	      uint32_t nexthop = std::get<2>(value);

	      // update tag
         ContraPacketTag contraPacketTag;
	      contraPacketTag.SetPid (pid);
	      contraPacketTag.SetTag (nexttag);
	      p->ReplacePacketTag (contraPacketTag);
	      
	      selectedoutputifc  = nexthop; 
      } else {
	      std::cout << "forwarding table miss" << dstid << std::endl;
      }
     
      // write hash
      if (selectedoutputifc > 0) {
            rtentry->SetOutputDevice (m_ipv4->GetNetDevice (selectedoutputifc));
            flowhash[dstid * CONTRA_HASH_SIZE + fh] = selectedoutputifc;
            flowtime[dstid * CONTRA_HASH_SIZE + fh] = curtime;
	         UpdatePortUtil(selectedoutputifc, p->GetSize());
      }else {
	      std::cout << "selectedoutputifc 0" << std::endl;
      }
      
   }

  return rtentry;

}

bool Contra::fromHost(Ptr<NetDevice> interface) {

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


int Contra::getdstid(ns3::Ipv4Address& dst) {
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
      std::cout << "Contra::getdstid() error" << std::endl;
      
    return result;

}

void Contra::sendcost(uint32_t origin, uint64_t mv, uint32_t pid, Ipv4Address senderAddress, uint32_t incomingInterface, uint64_t version) { 

   NS_LOG_FUNCTION (this << origin << mv << pid);

   std::map<int, std::map<int, uint64_t>>::iterator it1 = lastProbeVersion.find(origin);
   
   std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
      for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
          uint32_t ifc = std::get<0>(*it);
          uint64_t portProbeId = 0;
          if (it1 != lastProbeVersion.end()) {
            std::map<int, uint64_t> rcvprobes =  lastProbeVersion[origin];
            std::map<int, uint64_t>::iterator it2 = rcvprobes.find(ifc);
            if (it2 != rcvprobes.end())
               portProbeId = rcvprobes[ifc];
          }
          if (std::get<0>(*it) != incomingInterface && version > portProbeId) {
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

void Contra::sendcost2(Ptr<Socket> sendingSocket, uint32_t origin, uint64_t mv, uint32_t pid, const Ipv4Address &dstAdr, uint32_t tag, uint64_t version) {

  Ptr<Packet> p = Create<Packet> ();
  SocketIpTtlTag ttltag;
  p->RemovePacketTag (ttltag);
  ttltag.SetTtl (1);
  p->AddPacketTag (ttltag);

  ContraTag contraTag;
  contraTag.SetSimpleValue (1);
  p->AddPacketTag (contraTag);

  ContraUpdateHeader hdr;
  hdr.SetTag (tag);
  hdr.SetOrigin (origin);
  hdr.SetMv (mv);
  hdr.SetPid (pid);
  hdr.SetVersion (version);

  p->AddHeader (hdr);

  sendingSocket->SendTo (p, 0, InetSocketAddress (dstAdr, CONTRA_PORT));

}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

int Contra::CalcFlowHash (Ptr<const Packet> p, const Ipv4Header &header) {

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

   return h1 % CONTRA_HASH_SIZE; // prime number

}

void Contra::SetNeighbors(std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> neighbours) {
   switchNeighbours = neighbours;
   
   std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
   for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
   
      uint32_t i = std::get<0>(*it); 
      
      for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++) {
            Ipv4InterfaceAddress address = m_ipv4->GetAddress (i, j);
            if (address.GetScope() != Ipv4InterfaceAddress::HOST) {

               TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
               Ptr<Node> theNode = GetObject<Node> ();
               Ptr<Socket> socket = Socket::CreateSocket (theNode, tid);
               InetSocketAddress local = InetSocketAddress (address.GetLocal (), CONTRA_PORT);
               socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
               socket->Bind (local);
               socket->SetIpRecvTtl (true);
               m_sendSocketList[socket] = i;
               socket->SetRecvCallback (MakeCallback (&Contra::Receive, this));
               socket->SetRecvPktInfo (true);

            }
       }
   }
   // schedule probe sending
   Simulator::Schedule (Seconds (m_probePeriod), &Contra::GenerateProbe, this);
}

void Contra::SetDestinationNetworks(std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> networks) {
  switchNetworks = networks;
}


void Contra::Receive (Ptr<Socket> socket) {

   Address sender;
   Ptr<Packet> packet = socket->RecvFrom (sender);
   InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (sender);

   Ipv4Address senderAddress = senderAddr.GetIpv4 ();
   uint16_t senderPort = senderAddr.GetPort ();

   Ipv4PacketInfoTag interfaceInfo;
   if (!packet->RemovePacketTag (interfaceInfo))
      NS_ABORT_MSG ("Error CONTRA message.");

   uint32_t incomingIf = interfaceInfo.GetRecvIf ();
   Ptr<Node> node = this->GetObject<Node> ();
   Ptr<NetDevice> dev = node->GetDevice (incomingIf);
   uint32_t ipInterfaceIndex = m_ipv4->GetInterfaceForDevice (dev);

   ContraUpdateHeader hdr;
   packet->RemoveHeader (hdr);

   HandleUpdate(hdr, senderAddress, senderPort, ipInterfaceIndex); 

   return;

}

void Contra::HandleUpdate (ContraUpdateHeader hdr, Ipv4Address senderAddress, uint16_t senderPort, uint32_t incomingInterface) {


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

  KeyFwdT key = std::make_tuple (origin, tag, pid);

  uint32_t localTag = m_ipv4->GetObject<Node> ()->GetId ();
  if (tag != localTag) {
	std::cout << "bad tag" << std::endl;
  }
  
  uint32_t nbrtag = getNbrTag(incomingInterface);

  bool forwardupdate = false;   
  ValueFwdT newvalue = std::make_tuple (updatedMv, nbrtag, incomingInterface);
  if (nbrtag != UINT_MAX) {
	  if ( fwdT.find(key) == fwdT.end() ) {
		fwdT[key] = newvalue;
		bestT[origin] = key;
      forwardupdate = true;
	  } else {
		ValueFwdT value = fwdT[key];
		uint64_t tableMv = std::get<0>(value);
		uint32_t tableOutputIfc = std::get<2>(value);

      
      if (firstnewprobe) {
         fwdT[key] = newvalue;
			bestT[origin] = key;
			forwardupdate = true;
      }
         
      // update route
		if (tableOutputIfc == incomingInterface && updatedMv != tableMv) { // function f from the paper
			fwdT[key] = newvalue;
			bestT[origin] = key;
         forwardupdate = true;
		} 
		
		// change route
		if (tableOutputIfc != incomingInterface && updatedMv < tableMv) { // function f from the paper
			fwdT[key] = newvalue;
			bestT[origin] = key;
         forwardupdate = true;
		} 
		
		                  
	  }
      if (forwardupdate) {
         Ipv4Address nbradr = getNbrAdr(incomingInterface);
         uint32_t bestMv = updatedMv; 
         sendcost(origin, bestMv, pid, nbradr, incomingInterface, version);
      }
   } else {
	   std::cout << "getNbrTag error " << std::endl;
   }  


}

uint64_t Contra::getUpdatedMv(uint64_t rcvMv, uint32_t incomingInterface) {

  uint64_t localMv = GetPortUtil(incomingInterface);

  return std::max(localMv, rcvMv);

}

uint32_t Contra::getNbrTag(uint32_t incomingInterface) {

  std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
  for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
 
    if (std::get<0>(*it) == incomingInterface) { 
       return std::get<2>(*it);
    }
  }
  std::cout << "getNbrTag failed " << std::endl;
  return UINT_MAX;
}

Ipv4Address Contra::getNbrAdr(uint32_t incomingInterface) {

  std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>>::iterator it;
  for (it = switchNeighbours.begin(); it != switchNeighbours.end(); ++it){
 
    if (std::get<0>(*it) == incomingInterface) {
       return std::get<1>(*it);
    }
       
  }
  std::cout << "getNbrTag failed " << std::endl;
  Ipv4Address adr("0.0.0.0");
  return adr;
}


void Contra::UpdatePortUtil(uint32_t port, uint32_t packetLen) {

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

int64_t Contra::GetPortUtil(uint32_t port) {

	int64_t util = 0;

	if ( utilizationLocal.find(port) != utilizationLocal.end() ) {
		util = utilizationLocal[port];
	}
	return util;	

}

void Contra::SetProbePeriod(double probePeriod) {
  m_probePeriod = probePeriod; // Contra paper proposes 0.5 * MAXRTT and more, set to few times RTT in simulations;
  std::cout << "m_probePeriod " << m_probePeriod << std::endl;
  m_tau = 0.5 * m_probePeriod * 1000000000;
}


ContraRoutingTableEntry::ContraRoutingTableEntry (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface)
  : Ipv4RoutingTableEntry ( Ipv4RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, interface) ),
    m_status (CONTRA_INVALID), m_torId (0) {
}


void ContraRoutingTableEntry::SetRouteStatus (Status_e status) {
   if (m_status != status)
      m_status = status;
}

ContraRoutingTableEntry::Status_e ContraRoutingTableEntry::GetRouteStatus (void) const {
   return m_status;
}

void ContraRoutingTableEntry::SetDstId (int torId) {
  if (m_torId != torId)
      m_torId = torId;
}

int ContraRoutingTableEntry::GetDstId () const {
  return m_torId;
}

}
