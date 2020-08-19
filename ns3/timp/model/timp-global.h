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

#ifndef TIMP_GLOBAL_H
#define TIMP_GLOBAL_H

#include <vector>
#include <map>

#include "ns3/timp.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/node-container.h"

namespace ns3 {

class TimpGlobal : public Object
{
public:

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  TimpGlobal ();


  void Initialize (NodeContainer network, std::list< int > &torlist);

  void PrintStatistics ();

protected:

  virtual void DoDispose (void);

private:

  bool m_enabled;          
  std::vector< Ptr<Timp> > m_sabLayers;
  std::list< int > torsws;

  std::vector<Ptr<Timp>> rtrpointers;

  Ptr<Timp> GetTimpRouting (Ptr<Ipv4> ipv4) const;

};


} // namespace ns3

#endif

