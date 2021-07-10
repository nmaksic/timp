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


#include "timp.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/timp-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/loopback-net-device.h"
#include "ns3/packet.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include <algorithm>

#define TIMP_PORT 11001

#define FLOWLET_TIMEOUT 200

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Timp");

NS_OBJECT_ENSURE_REGISTERED (Timp);

Timp::Timp ()
  : m_ipv4 (0), m_initialized (false) {
  m_routes.clear();
  m_routeUpdates = 0;
  m_localUpdates = 0;
  m_generatedRouteUpdates = 0;
  m_hashRoundRobin = 0;
}

Timp::~Timp () {
}

TypeId
Timp::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::Timp")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<Timp> ()
    ;
  return tid;
}


void Timp::DoInitialize () {

   m_initialized = true;

   for (uint32_t i = 0 ; i < m_ipv4->GetNInterfaces (); i++) {
      if (DynamicCast<LoopbackNetDevice> (m_ipv4->GetNetDevice (i)))
         continue;

      m_ipv4->SetForwarding (i, true);

      /*for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++) {
         Ipv4InterfaceAddress address = m_ipv4->GetAddress (i, j);
         if (address.GetScope() != Ipv4InterfaceAddress::HOST) {

            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            Ptr<Node> theNode = GetObject<Node> ();
            Ptr<Socket> socket = Socket::CreateSocket (theNode, tid);
            InetSocketAddress local = InetSocketAddress (address.GetLocal (), TIMP_PORT);
            socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
            socket->Bind (local);
            socket->SetIpRecvTtl (true);
            m_sendSocketList[socket] = i;
            socket->SetRecvCallback (MakeCallback (&Timp::Receive, this));
            socket->SetRecvPktInfo (true);

         }
      }*/
   }

   Ipv4RoutingProtocol::DoInitialize ();
}

Ptr<Ipv4Route> Timp::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{

   Ipv4Address destination = header.GetDestination ();
   Ptr<Ipv4Route> rtentry = 0;

   rtentry = LookupTimp (destination, p, header, oif, true );

   if (rtentry)
      sockerr = Socket::ERROR_NOTERROR;
   else
      sockerr = Socket::ERROR_NOROUTETOHOST;

   return rtentry;
}

bool Timp::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
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

  Ptr<Ipv4Route> rtentry = LookupTimp (header.GetDestination (), p, header, 0, false  );

  if (rtentry != 0) {
      ucb (rtentry, p, header);  
      return true;
  } else
      return false; 
}

void Timp::NotifyInterfaceUp (uint32_t i) {
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
          InetSocketAddress local = InetSocketAddress (address.GetLocal (), TIMP_PORT);
          socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
          socket->Bind (local);
          socket->SetIpRecvTtl (true);
          m_sendSocketList[socket] = i;
          socket->SetRecvCallback (MakeCallback (&Timp::Receive, this));
          socket->SetRecvPktInfo (true);
       }

  }


}

void Timp::NotifyInterfaceDown (uint32_t interface) {

   for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) {
      if (iter->second == interface) {
         iter->first->Close ();
         m_sendSocketList.erase (iter);
         break;
      }
   }

}

void Timp::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Timp::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
}

void Timp::SetIpv4 (Ptr<Ipv4> ipv4) {
  NS_LOG_FUNCTION (this << ipv4);

  uint32_t i = 0;
  m_ipv4 = ipv4;

   for (i = 0; i < m_ipv4->GetNInterfaces (); i++)
      if (m_ipv4->IsUp (i))
         NotifyInterfaceUp (i);
      else
         NotifyInterfaceDown (i);

}

void Timp::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
}

void Timp::DoDispose () {
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


void Timp::PrintPrices(bool alternateonly, int routercount) {

  if (alternateonly) {
      std::cout << "ROUTER " << m_ipv4->GetObject<Node> ()->GetId () << ": " << m_ipv4->GetAddress (1, 0) << std::endl;
  }

  // prints alternate routes for each router, router by router
  for (int i = 0; i < routercount; i++) {
   std::cout << "Router " << i << std::endl;
   if (!alternateonly) {
      std::cout << " prices_prt [index port]: ";
      for (int j = 0; j < 10; j++)
         std::cout << prices_prt[j] << " ";
      std::cout << std::endl;
   }
   for (int j = 0; j < ALTRT_MAX; j++)
      if (alternaterutes[ALTRT_MAX * i + j] != 0) {
         std::cout << " Alternate port " << j << " prt " << alternaterutes[ALTRT_MAX * i + j] << std::endl;
         if (!alternateonly) {
            std::cout << "   prices_nbr[index dstid, start index " << (alternaterutes[ALTRT_MAX * i + j]-1)*DST_MAX <<  "]: ";
            for (int z = 0; z < 10; z++) 
               std::cout << prices_nbr[(alternaterutes[ALTRT_MAX * i + j]-1)*DST_MAX + z] << " ";
            std::cout << std::endl;
         }
      }


  }
   
}

void Timp::PrintStatistics() {

  
   std::cout << "ROUTER " << m_ipv4->GetObject<Node> ()->GetId () << ": " << m_ipv4->GetAddress (1, 0) << std::endl;

   for (int i = 0; i < PRT_MAX; i++)
      if (portflows[i] != 0) 
         std::cout << " Port " << i << " flows " << portflows[i] << std::endl;   


   for (int i = 0; i < PRT_MAX; i++)
      if (porthashes[i] != 0) 
         std::cout << " Port " << i << " porthashes " << porthashes[i] << std::endl;


   
}


Ptr<Ipv4Route> Timp::LookupTimp (Ipv4Address dst, Ptr<const Packet> p, const Ipv4Header &header, Ptr<NetDevice> interface, bool generatedpacket) {

  Ptr<Ipv4Route> rtentry = 0;
  //uint16_t longestMask = 0;
   rtentry = Create<Ipv4Route> ();
   Ipv4Address gw("0.0.0.0");
   rtentry->SetGateway (gw);

   /*TimpRoutingTableEntry* selectedroute = NULL;
   for (RoutesI it = m_routesTor.begin (); it != m_routesTor.end (); it++)
    {
      TimpRoutingTableEntry* j = it->first;
      if (j->GetRouteStatus () == TimpRoutingTableEntry::TIMP_VALID)
        {
          Ipv4Mask mask = j->GetDestNetworkMask ();
          uint16_t maskLen = mask.GetPrefixLength ();
          Ipv4Address entry = j->GetDestNetwork ();


          if (mask.IsMatch (dst, entry))
            {

              if (!interface || interface == m_ipv4->GetNetDevice (j->GetInterface ()))
                {
                  if (maskLen < longestMask)
                     continue;

                  longestMask = maskLen;

                  Ipv4RoutingTableEntry* route = j;
                  uint32_t interfaceIdx = route->GetInterface ();
                  rtentry = Create<Ipv4Route> ();

                  if (route->GetDest ().IsAny ()) 
                    {
                      rtentry->SetSource (m_ipv4->SourceAddressSelection (interfaceIdx, route->GetGateway ()));
                    }
                  else
                    {
                      rtentry->SetSource (m_ipv4->SourceAddressSelection (interfaceIdx, route->GetDest ()));
                    }

                  rtentry->SetDestination (route->GetDest ());
                  rtentry->SetGateway (route->GetGateway ());
                  rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));

                  NS_LOG_LOGIC ("interface " << interfaceIdx << " on port " << rtentry->GetOutputDevice ()->GetIfIndex() );

                  selectedroute = j;
                }
            }
        }
    }*/
    // found lpm, now check and update hash
    
    if (generatedpacket) {
   
      TimpTag tag;
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
                  
                  
                  if (header.GetProtocol() == 6) {
                     TcpHeader tcpHeader;
                     p->PeekHeader(tcpHeader);
                  
                     if ((tcpHeader.GetFlags ()) & TcpHeader::SYN) {
                        int dstid = m_ipv4->GetObject<Node> ()->GetId ();
                        sendcost(dstid, 0); // price 0
                        m_generatedRouteUpdates++; 
                        //synflags++;
                        //std::cout << "SYN flag " <<  << std::endl;
                     }
                  }
                  
                  // triger update of the entire tree
                  // no need because switches will do that for each part of the path
                  
                  /*int dstid = m_ipv4->GetObject<Node> ()->GetId ();
                  int fh = CalcFlowHash (p, header);
                  int64_t curtime = ns3::Simulator::Now().GetMicroSeconds ();
                  
                  //int outputifc = flowhash[dstid * DST_HASH_SIZE + fh];
                  int64_t elapsedtime = flowtime[dstid * DST_HASH_SIZE + fh];
                  
                  if (curtime < elapsedtime + 100) {
                     
                  } else {
                  
                     //if (elapsedtime > 0) {
                        flowtime[dstid * DST_HASH_SIZE + fh] = curtime;
                     //}
                     sendcost(dstid, 0); // price 0
                     m_generatedRouteUpdates++;
                  }*/
                  
                  return rtentry;
               }
            }  
         }
     }

   
   int dstid = getdstid(dst);
   
   if (dstid == -1)
      return NULL;
   // if (rtentry && !generatedpacket) {
   //   if (selectedroute != NULL) {

   //      int dstid = selectedroute->GetDstId();
   
         int fh = CalcFlowHash (p, header);
         int64_t curtime = ns3::Simulator::Now().GetMicroSeconds ();
    
         // check second hash table entry for expiration
         incHashRr();
         uint32_t ht1 = dstid * DST_HASH_SIZE + fh;
         if (m_hashRoundRobin == ht1)
            incHashRr();
         
         int outputifc = flowhash[m_hashRoundRobin];
         int64_t elapsedtime = flowtime[m_hashRoundRobin];

         if (elapsedtime != 0 && curtime >= elapsedtime + FLOWLET_TIMEOUT && outputifc > 0) {
            //uint32_t dstid1 = getRrDst();
            flowtime[dstid * DST_HASH_SIZE + fh] = 0;
            prices_prt[(outputifc-1)]--;
            //if (selectedoutputifc == outputifc)
            //   minprice--;
         }
   
         // choose next hop
         
        int selectedoutputifc=alternaterutes[ALTRT_MAX * dstid]; //selectedroute->GetInterface ();
        int price_nbr = prices_nbr[(selectedoutputifc-1)*DST_MAX + dstid];
        int price_prt = prices_prt[(selectedoutputifc-1)];
        int price = std::max(price_nbr, price_prt);

         int minprice = price;
         int maxprice = price;
         int lastprtprice = price_prt;

         for (int i = 1; i < ALTRT_MAX; i++) {
            int prt = alternaterutes[ALTRT_MAX * dstid + i];
            
            if (prt > 0) {
               price_nbr = prices_nbr[(prt-1)*DST_MAX + dstid];
               price_prt = prices_prt[(prt-1)];
               price = std::max(price_nbr, price_prt);
       
               // ako su cene jednake preferiramo rutu kojoj max util nije na susednom linku
               if (price < minprice || (price == minprice && price_prt < lastprtprice)) {
                  minprice = price;
                  selectedoutputifc = prt;
                  lastprtprice = price_prt;

               }
               if (price > maxprice) 
                  maxprice = price;
            }
         }
            
            
         

         
    
         /*
         if (m_ipv4->GetObject<Node> ()->GetId () == 0) {
         if (header.GetProtocol() == 6) {
         TcpHeader tcpHeader;
         p->PeekHeader(tcpHeader);
         uint16_tdstport = tcpHeader.GetDestinationPort ();
         uint16_t srcport = tcpHeader.GetSourcePort ();
         
         //if (dstport == 9000)
         //std::cout << "fh: " << fh << " dstport " << dstport << " srcport " << srcport << " simtime " << Simulator::Now().GetMicroSeconds () << std::endl;
         //}
         }
         }*/
         
         

         outputifc = flowhash[dstid * DST_HASH_SIZE + fh];
         elapsedtime = flowtime[dstid * DST_HASH_SIZE + fh];

         // flowlet gap 200us as in Contra paper
         if (curtime < elapsedtime + FLOWLET_TIMEOUT && outputifc > 0) {
            
            uint32_t interfaceIdx = outputifc;
            rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
            flowtime[dstid * DST_HASH_SIZE + fh] = curtime;

            porthashes[interfaceIdx]++;
            
            selectedoutputifc = outputifc;
            
         } else {
            // empty previous hash
            if (elapsedtime > 0) {
               flowtime[dstid * DST_HASH_SIZE + fh] = 0;
               prices_prt[(outputifc-1)]--;
               if (selectedoutputifc == outputifc)
                  minprice--;
            }
            minprice++;
            
            
            // write hash
            if (selectedoutputifc > 0) {
                  if (outputifc != selectedoutputifc || pricessent[dstid] != minprice) {
                     sendcost(dstid, minprice); 
                     m_localUpdates++;
                     m_generatedRouteUpdates++;
                  }
                  rtentry->SetOutputDevice (m_ipv4->GetNetDevice (selectedoutputifc));
                  flowhash[dstid * DST_HASH_SIZE + fh] = selectedoutputifc;
                  flowtime[dstid * DST_HASH_SIZE + fh] = curtime;
                  prices_prt[(selectedoutputifc-1)]++;
                  portflows[selectedoutputifc]++;
            }
            
         }

         
         
         
         
         
         //if (m_ipv4->GetObject<Node> ()->GetId () == 1) {
         //std::cout << "selectedoutputifc " << selectedoutputifc << std::endl;
        //}
         
   //   }
  //  }

  

  return rtentry;

}


void Timp::incHashRr() {

   //uint32_t dst = getHtDst(m_hashRoundRobin);

   
   m_hashRoundRobin++;
   //uint32_t dst1 = getHtDst(m_hashRoundRobin);
   //if (dst != dst1) {
   //   m_hashRoundRobin = dst * DST_HASH_SIZE;
   //}
   if (m_hashRoundRobin >= DST_HASH_SIZE*(m_swcount-1)) 
      m_hashRoundRobin = 0;
   
}

uint32_t Timp::getHtDst(uint32_t ht) {
   
   return ht / DST_HASH_SIZE;
   
}


int Timp::getdstid(ns3::Ipv4Address& dst) {
   int result = -1;
   uint16_t longestMask = 0;

   std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >>::iterator ita;
   for (ita = switchNetworks.begin (); ita != switchNetworks.end (); ita++)
    {
      //std::cout << ita->first << ": ";
      std::list<std::pair <Ipv4Address, Ipv4Mask>>::iterator ita1;
      for (ita1 = ita->second.begin (); ita1 != ita->second.end (); ita1++) {
         //std::cout << ita->first << ", " << ita1->first << ", " << ita1->second << std::endl;  
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
      std::cout << "Timp::getdstid() error" << std::endl;
      
    return result;

}


void Timp::sendcost(int dstId, int dstPrice) { 
   NS_LOG_FUNCTION (this << dstId << dstPrice);

  // we use one of the sending sockets, as they're bound to the right interface
  // and the local address might be used on different interfaces.

  if ( previousPorts.find(dstId) != previousPorts.end() ) {

      std::list<std::pair <uint32_t, std::string>>& lst = previousPorts[dstId];

      std::list<std::pair <uint32_t, std::string>>::iterator it;
      for (it = lst.begin(); it != lst.end(); ++it){
            // find socket and send
          Ptr<Socket> sendingSocket;
           for (SocketListI iter = m_sendSocketList.begin (); iter != m_sendSocketList.end (); iter++ ) {
               if (iter->second == it->first) {
                   sendingSocket = iter->first;
                   sendcost2(sendingSocket, dstId, dstPrice, it->second); 

                   pricessent[dstId] = dstPrice;
                 }
             }
      }
   } else {
      NS_LOG_LOGIC ("sendcost error");
   }  

}

  


void Timp::sendcost2(Ptr<Socket> sendingSocket, int dstId, int dstPrice, const std::string &adr) {

  
  Ptr<Packet> p = Create<Packet> ();
  SocketIpTtlTag tag;
  p->RemovePacketTag (tag);
  tag.SetTtl (1);
  p->AddPacketTag (tag);
  
  /*SocketIpTosTag tostag;
  p->RemovePacketTag (tostag);
  tostag.SetTos (Socket::NS3_PRIO_CONTROL);
  p->AddPacketTag (tostag);*/

  TimpTag timpTag;
  timpTag.SetSimpleValue (1);
  p->AddPacketTag (timpTag);

  Ipv4Address dstAdr(adr.c_str());

  TimpUpdateHeader hdr;
  hdr.SetCommand (TimpUpdateHeader::UPDATE);
  hdr.SetDstId (dstId);
  hdr.SetDstPrice (dstPrice);

  p->AddHeader (hdr);


  sendingSocket->SendTo (p, 0, InetSocketAddress (dstAdr, TIMP_PORT));
  p->RemoveHeader (hdr);

  m_routeUpdates++;
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

int Timp::CalcFlowHash (Ptr<const Packet> p, const Ipv4Header &header) {

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

   return h1 % DST_HASH_SIZE; // prime number

}


void Timp::AddNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface) {
  TimpRoutingTableEntry* route = new TimpRoutingTableEntry (network, networkPrefix, interface);
  route->SetRouteStatus (TimpRoutingTableEntry::TIMP_VALID);
  m_routes.push_back (std::make_pair (route, EventId ()));
}

void Timp::AddTorRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, int torid, uint32_t interface) {

  TimpRoutingTableEntry* route = new TimpRoutingTableEntry (network, networkPrefix, interface);
  route->SetRouteStatus (TimpRoutingTableEntry::TIMP_VALID);
  route->SetDstId (torid);
  m_routesTor.push_back (std::make_pair (route, EventId ()));

   /*if (m_ipv4->GetObject<Node> ()->GetId () == 0) {
      std::cout << "AddTorRouteTo torid" << torid << " interface " << interface << std::endl;
   }*/

  for (int i = 0; i < ALTRT_MAX; i++) {
     uint32_t prt = alternaterutes[ALTRT_MAX * torid + i];

     if (prt == interface)
         break;
     if (prt == 0) {
         alternaterutes[ALTRT_MAX * torid+i] = interface;
         //std::sort(alternaterutes + ALTRT_MAX * torid, alternaterutes + ALTRT_MAX * torid + i+1);
         break;
     }
  }
   

}

void Timp::AddPreviousPort (int dstid, uint32_t interface, std::string adr) {

   /*if (dstid == 0 && m_ipv4->GetObject<Node> ()->GetId () == 0) {
      std::cout << " AddPreviousPort " << interface << std::endl;
   }*/

   if ( previousPorts.find(dstid) == previousPorts.end() ) 
      previousPorts.insert ( std::pair<int,std::list<std::pair <uint32_t, std::string>>>(dstid,std::list<std::pair <uint32_t, std::string>>()) );
      

   std::list<std::pair<uint32_t, std::string>>& lst = previousPorts[dstid];
   unsigned int ifc = (unsigned int)interface;
   auto it =  std::find_if(lst.begin(), lst.end(), [&ifc](const std::pair<uint32_t, std::string>& element){ return element.first == ifc;});
   bool found = (it != lst.end());

   if (!found)
      lst.push_back(std::make_pair (interface, adr));

}


void Timp::Receive (Ptr<Socket> socket) {

   Address sender;
   Ptr<Packet> packet = socket->RecvFrom (sender);
   InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (sender);

   Ipv4Address senderAddress = senderAddr.GetIpv4 ();
   uint16_t senderPort = senderAddr.GetPort ();

   Ipv4PacketInfoTag interfaceInfo;
   if (!packet->RemovePacketTag (interfaceInfo))
      NS_ABORT_MSG ("Error TIMP message.");

   uint32_t incomingIf = interfaceInfo.GetRecvIf ();
   Ptr<Node> node = this->GetObject<Node> ();
   Ptr<NetDevice> dev = node->GetDevice (incomingIf);
   uint32_t ipInterfaceIndex = m_ipv4->GetInterfaceForDevice (dev);

   TimpUpdateHeader hdr;
   packet->RemoveHeader (hdr);

   if (hdr.GetCommand () == TimpUpdateHeader::UPDATE) {
      HandleUpdate(hdr, senderAddress, senderPort, ipInterfaceIndex); 
      
      // remove header
      //UdpHeader udpHeader;
      //packet->RemoveHeader (udpHeader);

      // remove header
      //Ipv4Header ipHeader;
      //packet->RemoveHeader (ipHeader);
      
      //std::cout << "tos " << ipHeader.GetTos() << std::endl;
      
   }

  return;

}

void Timp::HandleUpdate (TimpUpdateHeader hdr, Ipv4Address senderAddress, uint16_t senderPort, uint32_t incomingInterface) {
   // update local and send further - send only up the tree to avoid loops
   // get price, update in arrays and send change if it happens
   uint32_t dstId = hdr.GetDstId ();
   uint32_t dstPrice = hdr.GetDstPrice ();


   prices_nbr[(incomingInterface-1)*DST_MAX + dstId] = dstPrice;
     
   int minprice = 1000001;
   int lastprtprice = 1000000;
   for (int i = 0; i < ALTRT_MAX; i++) {
      int prt = alternaterutes[ALTRT_MAX * dstId + i];
      if (prt > 0) {
         int price_nbr = prices_nbr[(prt-1)*DST_MAX + dstId];
         int price_prt = prices_prt[(prt-1)];
         int price = price_nbr + price_prt;
         if (price < minprice || (price == minprice && price_prt < lastprtprice)) {
            minprice = price;
            lastprtprice = price_prt;
         }
      }
   }

   if (minprice != pricessent[dstId]) {
      sendcost(dstId, dstPrice);
   }
}


uint32_t Timp::GetLocalUpdates() {
   return m_localUpdates;
}

uint32_t Timp::GetRouteUpdates() {
   return m_routeUpdates;
}

uint32_t Timp::GetGeneratedRouteUpdates() {
   return m_generatedRouteUpdates;
}

void Timp::SetNeighbors(std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> neighbours) {
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
               InetSocketAddress local = InetSocketAddress (address.GetLocal (), TIMP_PORT);
               socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
               socket->Bind (local);
               socket->SetIpRecvTtl (true);
               m_sendSocketList[socket] = i;
               socket->SetRecvCallback (MakeCallback (&Timp::Receive, this));
               socket->SetRecvPktInfo (true);
               //socket->SetPriority(Socket::NS3_PRIO_CONTROL);
               //socket->SetIpTos(Socket::NS3_PRIO_CONTROL);

            }
       }
   }
}

void Timp::SetDestinationNetworks(std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> networks, uint32_t swcount) {
  switchNetworks = networks;
  m_swcount = swcount;
}


TimpRoutingTableEntry::TimpRoutingTableEntry (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface)
  : Ipv4RoutingTableEntry ( Ipv4RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, interface) ),
    m_status (TIMP_INVALID), m_torId (0) {
}


void TimpRoutingTableEntry::SetRouteStatus (Status_e status) {
   if (m_status != status)
      m_status = status;
}

TimpRoutingTableEntry::Status_e TimpRoutingTableEntry::GetRouteStatus (void) const {
   return m_status;
}

void TimpRoutingTableEntry::SetDstId (int torId) {
  if (m_torId != torId)
      m_torId = torId;
}

int TimpRoutingTableEntry::GetDstId () const {
  return m_torId;
}




}
