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

#include "contra-global.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <fstream>
#include <sstream>
#include "ns3/ipv4-list-routing.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ContraGlobal")
  ;

NS_OBJECT_ENSURE_REGISTERED (ContraGlobal)
  ;


TypeId 
ContraGlobal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContraGlobal")
    .SetParent<Object> ()
    .AddConstructor<ContraGlobal> ();
  return tid;
}

TypeId 
ContraGlobal::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

ContraGlobal::ContraGlobal ()
  : m_enabled (false)
{
}

void
ContraGlobal::DoDispose (void)
{
  for (uint32_t i = 0; i < m_sabLayers.size (); i++)
    {
      m_sabLayers[i]->Dispose ();
      m_sabLayers[i] = 0;
    }
  Object::DoDispose (); 
}


Ptr<Contra>
ContraGlobal::GetContraRouting (Ptr<Ipv4> ipv4) const
{
   NS_LOG_FUNCTION (this);
   Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
   if (DynamicCast<Contra> (ipv4rp))
      return DynamicCast<Contra> (ipv4rp); 
   if (DynamicCast<Ipv4ListRouting> (ipv4rp)){
      Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
      int16_t priority;
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++) {
         Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
         if (DynamicCast<Contra> (temp)) 
            return DynamicCast<Contra> (temp);
      }
   }
   return 0;
}


// keep devices in order to transform addresses to identifiers
void
ContraGlobal::Initialize (NodeContainer switches, std::list< int > &torlist, std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >> switchNeighbours, 
                           std::list<std::pair <int, std::list<std::pair <Ipv4Address, Ipv4Mask>> >> switchNetworks, double probePeriod)
{

      // initialize switches with networks and neighbors
      for(NodeContainer::Iterator sw = switches.Begin ();  sw != switches.End (); sw++)
       {
       int dstid = (*sw)->GetId();
       Ptr<Ipv4> ipv4 = (*sw)->GetObject<Ipv4>();
       Ptr<Contra> contra = GetContraRouting(ipv4);
       
       contra->SetProbePeriod(probePeriod);
       
       // set neighbors
       std::list<std::pair <int, std::list<std::tuple <uint32_t, Ipv4Address, uint32_t>> >>::iterator it;
         for (it = switchNeighbours.begin (); it != switchNeighbours.end (); it++)
          {
            if (it->first == dstid) {
               if (contra != 0) {  
                     contra->SetNeighbors(it->second);
               } 
            }
          }
       
         // set destination networks
         contra->SetDestinationNetworks(switchNetworks);
         rtrpointers.push_back(contra);
       }       
       

}

void
ContraGlobal::PrintStatistics ()
{
   for(std::vector<Ptr<Contra>>::iterator it = rtrpointers.begin(); it != rtrpointers.end(); ++it) 
      (*it)->PrintStatistics();
}

} // namespace ns3

