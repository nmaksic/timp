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

#ifndef DASH_GLOBAL_H
#define DASH_GLOBAL_H

#include <vector>
#include <map>
#include <tuple>

#include "ns3/dash.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/node-container.h"

namespace ns3 {

class DashGlobal : public Object
{
public:

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  DashGlobal ();


  void Initialize (NodeContainer switches, std::list< int > &torlist, std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >> switchNeighbours, 
                           std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> switchNetworks, double probePeriod, bool packetrouting, int topology);

  void PrintStatistics ();

protected:

  virtual void DoDispose (void);

private:

  bool m_enabled;          
  std::vector< Ptr<Dash> > m_sabLayers;
  std::list< int > torsws;

  std::vector<Ptr<Dash>> rtrpointers;

  Ptr<Dash> GetDashRouting (Ptr<Ipv4> ipv4) const;

};


} // namespace ns3

#endif

