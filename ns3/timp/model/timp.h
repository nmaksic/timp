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

#ifndef TIMP_H
#define TIMP_H

#include <list>

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timp-header.h"

namespace ns3 {


#define DST_HASH_SIZE 97
#define DST_MAX 100
#define PRT_MAX 20

#define ALTRT_MAX 50


class TimpRoutingTableEntry : public Ipv4RoutingTableEntry
{
public:

  enum Status_e {
    TIMP_VALID,
    TIMP_INVALID,
  };


  TimpRoutingTableEntry (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface);

  void SetRouteStatus (Status_e status);

  Status_e GetRouteStatus (void) const;

  void SetDstId(int torId);

  int GetDstId (void) const;

private:
  Status_e m_status; 
  int m_torId;
};

class Timp : public Ipv4RoutingProtocol
{
public:

  Timp ();
  virtual ~Timp ();

  static TypeId GetTypeId (void);

  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  void AddNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface);

  void AddTorRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, int torid, uint32_t interface);

  void AddPreviousPort (int dstid, uint32_t interface, std::string adr);

  void PrintPrices(bool alternateonly, int routercount);

  void PrintStatistics();

protected:
  virtual void DoDispose ();

  void DoInitialize ();

private:
  typedef std::list<std::pair <TimpRoutingTableEntry *, EventId> > Routes;

  typedef std::list<std::pair <TimpRoutingTableEntry *, EventId> >::iterator RoutesI;

  typedef std::list<std::pair <TimpRoutingTableEntry *, EventId> > RoutesTor;

  void Receive (Ptr<Socket> socket);

  Ptr<Ipv4Route> LookupTimp (Ipv4Address dst, Ptr<const Packet> p, const Ipv4Header &header, Ptr<NetDevice> interface, bool generatedpacket);

  int CalcFlowHash (Ptr<const Packet> p, const Ipv4Header &header);

  void sendcost(int dstId, int dstPrice);
  void sendcost2(Ptr<Socket> sendingSocket, int dstId, int dstPrice, const  std::string &adr);

  void HandleUpdate (TimpUpdateHeader requestHdr, Ipv4Address senderAddress, uint16_t senderPort, uint32_t incomingInterface);

  Routes m_routes; 
  Ptr<Ipv4> m_ipv4; 

  typedef std::map< Ptr<Socket>, uint32_t> SocketList;

  typedef std::map<Ptr<Socket>, uint32_t>::iterator SocketListI;

  SocketList m_sendSocketList; 
  SocketList m_recvSocketList; 

  EventId m_nextUnsolicitedUpdate; 
  EventId m_nextTriggeredUpdate; 

  std::map<uint32_t, uint8_t> m_interfaceMetrics; 

  bool m_initialized; 
  uint32_t m_linkDown; 

  RoutesTor m_routesTor; 

  int flowhash[DST_HASH_SIZE * DST_MAX];
  int64_t flowtime[DST_HASH_SIZE * DST_MAX];
  int prices_nbr[PRT_MAX * DST_MAX];
  int prices_prt[PRT_MAX];
  uint32_t alternaterutes[ALTRT_MAX * DST_MAX];
  int checkstaledst;
  int stalecheckpos[DST_MAX];
  std::map<int, std::list<std::pair <uint32_t, std::string>> > previousPorts; 
  int pricessent[DST_MAX];
  long portflows[PRT_MAX];
  long porthashes[PRT_MAX];
};

} 
#endif /* TIMP_H */

