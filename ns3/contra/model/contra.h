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

#ifndef CONTRA_H
#define CONTRA_H

#include <list>
#include <tuple>

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/random-variable-stream.h"
#include "ns3/contra-header.h"

namespace ns3 {


#define CONTRA_HASH_SIZE 997
#define DST_MAX 100
#define PRT_MAX 20

typedef std::tuple<uint32_t, uint32_t, uint32_t> KeyFwdT; // dst, tag - this node, pid - 0
typedef std::tuple<uint64_t, uint32_t, uint32_t> ValueFwdT; // mv - metrics vector - utilization, ntag - next node, nhop - outputport 

class ContraRoutingTableEntry : public Ipv4RoutingTableEntry
{
public:

  enum Status_e {
    CONTRA_VALID,
    CONTRA_INVALID,
  };


  ContraRoutingTableEntry (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface);

  void SetRouteStatus (Status_e status);

  Status_e GetRouteStatus (void) const;

  void SetDstId(int torId);

  int GetDstId (void) const;

private:
  Status_e m_status; 
  int m_torId;
};

class Contra : public Ipv4RoutingProtocol
{
public:

  Contra ();
  virtual ~Contra ();

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

  void PrintPrices(bool alternateonly, int routercount);

  void PrintStatistics();

  void SetNeighbors(std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> neighbours);
  void SetDestinationNetworks(std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> networks);

  void SetProbePeriod(double probePeriod);

protected:
  virtual void DoDispose ();

  void DoInitialize ();

private:
  typedef std::list<std::pair <ContraRoutingTableEntry *, EventId> > Routes;

  typedef std::list<std::pair <ContraRoutingTableEntry *, EventId> >::iterator RoutesI;

  typedef std::list<std::pair <ContraRoutingTableEntry *, EventId> > RoutesTor;

  void Receive (Ptr<Socket> socket);

  Ptr<Ipv4Route> LookupContra (Ipv4Address dst, Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> interface, bool generatedpacket);

  int CalcFlowHash (Ptr<const Packet> p, const Ipv4Header &header);

  void sendcost(uint32_t origin, uint64_t mv, uint32_t pid, Ipv4Address senderAddress, uint32_t incomingInterface, uint64_t version);
  void sendcost2(Ptr<Socket> sendingSocket, uint32_t origin, uint64_t mv, uint32_t pid, const Ipv4Address &dstAdr, uint32_t tag, uint64_t version);

  void HandleUpdate (ContraUpdateHeader requestHdr, Ipv4Address senderAddress, uint16_t senderPort, uint32_t incomingInterface);

  int getdstid(ns3::Ipv4Address& dst);
  
  void GenerateProbe();

  void UpdatePortUtil(uint32_t port, uint32_t packetLen);
  int64_t GetPortUtil(uint32_t port);

  bool fromHost(Ptr<NetDevice> interface);

  uint64_t getUpdatedMv(uint64_t rcvMv, uint32_t incomingInterface);
  uint32_t getNbrTag(uint32_t incomingInterface);
  Ipv4Address getNbrAdr(uint32_t incomingInterface);

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

  int flowhash[CONTRA_HASH_SIZE * DST_MAX];
  int64_t flowtime[CONTRA_HASH_SIZE * DST_MAX];
  int checkstaledst;
  int stalecheckpos[DST_MAX];
  long portflows[PRT_MAX];
  long porthashes[PRT_MAX];
  uint64_t m_probeVersionCounter;
  bool m_useUtilization = true;
  bool m_sendProbes = true;
  uint64_t m_tau;
  double m_maxRtt;
  double m_probePeriod;

  std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> switchNeighbours; // port id, neigbout adr, neigbour id
  std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> switchNetworks;
  std::map<int, std::map<int, uint64_t>> lastProbeVersion; // dst, port, lastprobe
  std::map<int, uint64_t> lastProbeVer; // dst, lastprobe
  std::map<int, std::pair <uint32_t, uint64_t>> minRcvPrices; // dst, min port, min price  supports both metrics
  std::map<uint32_t, uint32_t> portUtil; // port, utilization 
  std::map<uint32_t, int64_t> lastPacketTime; // port, time   
  std::map<uint32_t, int64_t> utilizationLocal; // port, util    
  std::map<int, uint32_t> lastProbeId;

  std::map<KeyFwdT, ValueFwdT> fwdT; // the forwarding table
  std::map<uint32_t, KeyFwdT> bestT; // the best path in the first switch of the flow

};

} 
#endif /* CONTRA_H */

